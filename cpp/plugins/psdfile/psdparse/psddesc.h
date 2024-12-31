#ifndef __psddesc_h__
#define __psddesc_h__

#include "psdbase.h"

#include <stdarg.h>
#include <map>
#include <vector>

namespace psd {
  // --------------------------------------------------------------------------
  // 定数
  // --------------------------------------------------------------------------
  
  // ディスクリプタアイテムタイプ
  enum DescriptorType {
    TYPE_REFERENCE      = 'obj ',
    TYPE_DESCRIPTOR     = 'Objc',
    TYPE_LIST           = 'VlLs',
    TYPE_DOUBLE         = 'doub',
    TYPE_UNIT_FLOAT     = 'UntF',
    TYPE_STRING         = 'TEXT',
    TYPE_ENUMERATED     = 'enum',
    TYPE_INTEGER        = 'long',
    TYPE_BOOLEAN        = 'bool',
    TYPE_GLOBAL_OBJECT  = 'GlbO',
    TYPE_CLASS1         = 'type',
    TYPE_CLASS2         = 'GlbC',
    TYPE_ALIAS          = 'alis',
    TYPE_RAW_DATA       = 'tdta',
  };

  enum DescriptorUnit {
    UNIT_POINTS       = '#Pnt',
    UNIT_MILLIMETERS  = '#Mlm',
    UNIT_ANGLE        = '#Ang', // degrees
    UNIT_DENSITY      = '#Rsl', // base per inch
    UNIT_DISTANCE     = '#Rlt', // base 72ppi
    UNIT_NONE         = '#Nne',
    UNIT_PERCENT      = '#Prc',
    UNIT_PIXELS       = '#Pxl',
  };

  // Reference type
  enum ReferenceType {
    REF_TYPE_PROPERTY     = 'prop',
    REF_TYPE_CLASS        = 'Clss',
    REF_TYPE_ENUM_REF     = 'Enmr',
    REF_TYPE_OFFSET       = 'rele',
    REF_TYPE_IDENTIFIER   = 'Idnt',
    REF_TYPE_INDEX        = 'indx',
    REF_TYPE_NAME         = 'name',
  };

  inline void indent_print(int indent, const char *fmt, ...) {
#ifdef _DEBUG
    dprint("%*s", indent, "");
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
#endif
  }

  // --------------------------------------------------------------------------
  // アイテム基底
  // --------------------------------------------------------------------------

  // ディスクリプタアイテム基底
  struct DescriptorItem {
    DescriptorItem(DescriptorType t) : type(t), isValid(true) {}
    virtual ~DescriptorItem() {}
    virtual bool load(IteratorBase *data) = 0;
    virtual void dump(int indent) = 0;
    const char *typeName() {
      const char *s = 0;
      switch (type) {
      case TYPE_REFERENCE:     s = "Reference";   break;
      case TYPE_DESCRIPTOR:    s = "Descriptor";  break;
      case TYPE_LIST:          s = "List";        break;
      case TYPE_DOUBLE:        s = "Double";      break;
      case TYPE_UNIT_FLOAT:    s = "UnitFloat";   break;
      case TYPE_STRING:        s = "String";      break;
      case TYPE_ENUMERATED:    s = "Enumrated";   break;
      case TYPE_INTEGER:       s = "Integer";     break;
      case TYPE_BOOLEAN:       s = "Boolean";     break;
      case TYPE_GLOBAL_OBJECT: s = "GlobalObj";   break;
      case TYPE_CLASS1:        s = "Class1";      break;
      case TYPE_CLASS2:        s = "Class2";      break;
      case TYPE_ALIAS:         s = "Alias";       break;
      case TYPE_RAW_DATA:      s = "RawData";     break;
      default:                 s= "Unknown";      break;
      };
      return s;
    }

    bool        isValid;
    DescriptorType     type;
  };

  // リファレンスアイテム基底
  struct ReferenceItem {
    ReferenceItem(ReferenceType t) : type(t) {}
    virtual ~ReferenceItem() {}
    virtual bool load(IteratorBase *data) = 0;
    virtual void dump(int indent) {};

    ReferenceType type;
  };

  // --------------------------------------------------------------------------
  // ディスクリプタ
  // --------------------------------------------------------------------------

  struct Descriptor;
  struct DescriptorList;
  struct DescriptorReference;
  struct DescriptorDouble;
  struct DescriptorUnitFloat;
  struct DescriptorString;
  struct DescriptorEnumerated;
  struct DescriptorInteger;
  struct DescriptorBoolean;
  struct DescriptorClass;
  struct DescriptorAlias;
  struct DescriptorRawData;

  // ディスクリプタ(自身もディスクリプタアイテムとなるケースがある)
  struct Descriptor : DescriptorItem {
    typedef std::map<std::string, DescriptorItem*> ItemMap;

    Descriptor(DescriptorType type=TYPE_DESCRIPTOR) : DescriptorItem(type) {}
    virtual ~Descriptor() {
      for (ItemMap::iterator it = itemMap.begin(); it != itemMap.end(); it++)	{
        delete it->second;
      }
    }
    virtual bool load(IteratorBase *data);
    virtual void dump(int indent=0) {
      dprint("Descriptor (size:%d, class:%s, name:%ls)\n",
             itemCount(), classId.c_str(), name.c_str());
      int i = 0;
      for (ItemMap::iterator it = itemMap.begin(); it != itemMap.end(); it++, i++)	{
        indent_print(indent+1, "[%s] ", it->first.c_str());
        it->second->dump(indent+2);
      }
    }
    int itemCount() {
      return (int)itemMap.size();
    }
    DescriptorItem *findItem(std::string key) {
      ItemMap::const_iterator it = itemMap.find(key);
      if (it != itemMap.end()) {
        return it->second;
      } else {
        return 0;
      }
    }

    // 戻り値オーバーロード用のクラス
    struct _finder {
      _finder(const ItemMap& itemMap, const std::string key)
      : itemMap(itemMap), key(key) { }
      DescriptorItem *find() const {
        ItemMap::const_iterator it = itemMap.find(key);
        return it != itemMap.end() ? it->second : 0;
      }
#define FINDER(type) operator type*() const { return (type*)find(); }
      FINDER(Descriptor);
      FINDER(DescriptorList);
      FINDER(DescriptorReference);
      FINDER(DescriptorDouble);
      FINDER(DescriptorUnitFloat);
      FINDER(DescriptorString);
      FINDER(DescriptorEnumerated);
      FINDER(DescriptorInteger);
      FINDER(DescriptorBoolean);
      FINDER(DescriptorClass);
      FINDER(DescriptorAlias);
      FINDER(DescriptorRawData);

    private:
      const ItemMap& itemMap;
      const std::string key;
    };
    _finder item(std::string key) const {
      return _finder(itemMap, key);
    }

    std::wstring name;
    std::string  classId;
    ItemMap      itemMap;
  };

  struct DescriptorReference : DescriptorItem {
    DescriptorReference() : DescriptorItem(TYPE_RAW_DATA) {}
    virtual ~DescriptorReference() {
      for (int i = 0; i < (int)items.size(); i++) {
        delete items[i];
      }
    }
    virtual bool load(IteratorBase *data);
    virtual void dump(int indent) {
      dprint("%s (count:%d)\n", typeName(), items.size());
      for (int i = 0; i < (int)items.size(); i++) {
        items[i]->dump(indent+1);
      }
    }

    int type;
    std::vector<ReferenceItem*> items;
  };

  struct DescriptorList : DescriptorItem {
    DescriptorList() : DescriptorItem(TYPE_LIST) {}
    virtual ~DescriptorList() {
      for (int i = 0; i < (int)items.size(); i++) {
        delete items[i];
      }
    }
    virtual bool load(IteratorBase *data);
    virtual void dump(int indent) {
      dprint("%s (count:%d)\n", typeName(), items.size());
      for (int i = 0; i < (int)items.size(); i++) {
        indent_print(indent+1, "[%d] ", i);
        items[i]->dump(indent+2);
      }
    }
    int itemCount() {
      return (int)items.size();
    }

    // 戻り値オーバーロード用のクラス
    struct _getter {
      _getter(const std::vector<DescriptorItem*>& items, int id)
      : items(items), id(id) { }
      DescriptorItem *get() const {
        return (id >= 0 && id <= (int)items.size()) ? items[id] : 0;
      }
#define GETTER(type) operator type*() const { return (type*)get(); }
      GETTER(Descriptor);
      GETTER(DescriptorList);
      GETTER(DescriptorReference);
      GETTER(DescriptorDouble);
      GETTER(DescriptorUnitFloat);
      GETTER(DescriptorString);
      GETTER(DescriptorEnumerated);
      GETTER(DescriptorInteger);
      GETTER(DescriptorBoolean);
      GETTER(DescriptorClass);
      GETTER(DescriptorAlias);
      GETTER(DescriptorRawData);

    private:
      const std::vector<DescriptorItem*>& items;
      int id;
    } item(int id) const {
      return _getter(items, id);
    }

    std::vector<DescriptorItem*> items;
  };

  struct DescriptorDouble : DescriptorItem {
    DescriptorDouble() : DescriptorItem(TYPE_DOUBLE) {}
    virtual bool load(IteratorBase *data);
    virtual void dump(int indent) {
      dprint("%s (val:%f)\n", typeName(), val);
    }

    float64_t val;
  };

  struct DescriptorUnitFloat : DescriptorItem {
    DescriptorUnitFloat() : DescriptorItem(TYPE_UNIT_FLOAT) {}
    virtual bool load(IteratorBase *data);
    virtual void dump(int indent) {
      const char *unitStr = 0;
      switch (unit) {
      case UNIT_POINTS:      unitStr = "Points";                  break;
      case UNIT_MILLIMETERS: unitStr = "MilliMeters";             break;
      case UNIT_ANGLE:       unitStr = "Angle(degree)";           break;
      case UNIT_DENSITY:     unitStr = "Density(base per inch)";  break;
      case UNIT_DISTANCE:    unitStr = "Distance(base 72ppi)";    break;
      case UNIT_NONE:        unitStr = "None";                    break;
      case UNIT_PERCENT:     unitStr = "Percent";                 break;
      case UNIT_PIXELS:      unitStr = "Pixels";                  break;
      default:               unitStr = "Unknown";                 break;
      }
      indent_print(indent, "%s (unit:%s, val:%f)\n", typeName(), unitStr, val);
    }

    DescriptorUnit unit;
    float64_t      val;
  };
  
  struct DescriptorString : DescriptorItem {
    DescriptorString() : DescriptorItem(TYPE_STRING) {}
    virtual bool load(IteratorBase *data);
    virtual void dump(int indent) {
      dprint("%s (val:%ls)\n", typeName(), val.c_str());
    }

    std::wstring val;
  };
  
  struct DescriptorEnumerated : DescriptorItem {
    DescriptorEnumerated() : DescriptorItem(TYPE_ENUMERATED) {}
    virtual bool load(IteratorBase *data);
    virtual void dump(int indent) {
      dprint("%s (type:%s, enum:%s)\n",
             typeName(), typeId.c_str(), enumId.c_str());
    }

    std::string  typeId;
    std::string  enumId;
  };

  struct DescriptorInteger : DescriptorItem {
    DescriptorInteger() : DescriptorItem(TYPE_INTEGER) {}
    virtual bool load(IteratorBase *data);
    virtual void dump(int indent) {
      dprint("%s (val:%d(0x%x))\n", typeName(), val, val);
    }

    int32_t val;
  };

  struct DescriptorBoolean : DescriptorItem {
    DescriptorBoolean() : DescriptorItem(TYPE_BOOLEAN) {}
    virtual bool load(IteratorBase *data);
    virtual void dump(int indent) {
      dprint("%s (val: %d)\n", typeName(), val);
    }

    bool val;
  };

  struct DescriptorClass : DescriptorItem {
    DescriptorClass(DescriptorType type) : DescriptorItem(type) {}
    virtual bool load(IteratorBase *data);
    virtual void dump(int indent) {
      dprint("%s (name:%ls, class:%s)\n",
             typeName(), name.c_str(), classId.c_str());
    }

    std::wstring name;
    std::string  classId;
  };

  struct DescriptorAlias : DescriptorItem {
    DescriptorAlias() : DescriptorItem(TYPE_ALIAS) {}
    virtual bool load(IteratorBase *data);
    virtual void dump(int indent) {
      dprint("%s (val:%s)\n", typeName(), alias.c_str());
    }

    // TODO string でいいか不明(unicodeの可能性も？)
    std::string alias;
  };

  // Raw は値の解釈がデータ次第でサイズがわからないので失敗扱いにしている
  struct DescriptorRawData : DescriptorItem {
    DescriptorRawData(IteratorBase *data) : DescriptorItem(TYPE_RAW_DATA) {}
    virtual bool load(IteratorBase *data) { return false; }
    virtual void dump(int indent) {
      dprint("%s\n", typeName());
    }
  };

  // --------------------------------------------------------------------------
  // リファレンス
  // --------------------------------------------------------------------------

  struct ReferenceProperty : ReferenceItem {
    ReferenceProperty() : ReferenceItem(REF_TYPE_PROPERTY) {}
    virtual bool load(IteratorBase *data);

    std::wstring name;
    std::string  classId;
    std::string  keyId;
  };

  struct ReferenceClass : ReferenceItem {
    ReferenceClass() : ReferenceItem(REF_TYPE_CLASS) {}
    virtual bool load(IteratorBase *data);

    std::wstring name;
    std::string  classId;
  };

  struct ReferenceEnumRef : ReferenceItem {
    ReferenceEnumRef() : ReferenceItem(REF_TYPE_ENUM_REF) {}
    virtual bool load(IteratorBase *data);

    std::wstring name;
    std::string  classId;
    std::string  typeId;
    std::string  enumId;
  };

  struct ReferenceOffset : ReferenceItem {
    ReferenceOffset() : ReferenceItem(REF_TYPE_OFFSET) {}
    virtual bool load(IteratorBase *data);

    std::wstring name;
    std::string  classId;
    int          offset;
  };

  struct ReferenceIdentifier : ReferenceItem {
    ReferenceIdentifier() : ReferenceItem(REF_TYPE_IDENTIFIER) {}
    virtual bool load(IteratorBase *data);

    int identifier;
  };

  struct ReferenceIndex : ReferenceItem {
    ReferenceIndex() : ReferenceItem(REF_TYPE_INDEX) {}
    virtual bool load(IteratorBase *data);

    int index;
  };

  struct ReferenceName : ReferenceItem {
    ReferenceName() : ReferenceItem(REF_TYPE_NAME) {}
    virtual bool load(IteratorBase *data);

    std::wstring name;
  };

} // namespace psd

#endif // __psddesc_h__
