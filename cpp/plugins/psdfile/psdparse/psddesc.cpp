#include "psddesc.h"

namespace psd {

    // --------------------------------------------------------------------------
    // ユーティリティ
    // --------------------------------------------------------------------------

    // 4バイトキー or stringタイプのID取得用ユーティリティ
    static void readId(IteratorBase *data, std::string &id) {
        int idSize = data->getInt32();
        if(idSize == 0) {
            idSize = 4;
        }
        std::vector<char> buf(idSize + 1);
        data->getData(&buf[0], idSize);
        buf[idSize] = '\0';
        id.assign(&buf[0]);
    }

    // タイプにしたがってアイテム取得をディスパッチ
    static DescriptorItem *readItem(IteratorBase *data) {
        DescriptorItem *item = 0;
        DescriptorType type = (DescriptorType)data->getInt32();
        switch(type) {
            case 'obj ':
                item = new DescriptorReference();
                break;
            case 'GlbO':
            case 'Objc':
                item = new Descriptor(type);
                break;
            case 'VlLs':
                item = new DescriptorList();
                break;
            case 'doub':
                item = new DescriptorDouble();
                break;
            case 'UntF':
                item = new DescriptorUnitFloat();
                break;
            case 'TEXT':
                item = new DescriptorString();
                break;
            case 'enum':
                item = new DescriptorEnumerated();
                break;
            case 'long':
                item = new DescriptorInteger();
                break;
            case 'bool':
                item = new DescriptorBoolean();
                break;
            case 'type':
            case 'GlbC':
                item = new DescriptorClass(type);
                break;
            case 'alis':
                item = new DescriptorAlias();
                break;
            case 'tdta':
                // undocumented
                // なうえ、サイズ情報もなくスキップできないので
                // これが出てきたらこれ以上はパースできない
                // item = new DescriptorRawData(data);
                break;
            default:
                break;
        }
        if(item) {
            item->load(data);
        }
        return item;
    }

    // --------------------------------------------------------------------------
    // ローダ
    // --------------------------------------------------------------------------
    bool Descriptor::load(IteratorBase *data) {
        data->getUnicodeString(name);
        readId(data, classId);

        int itemCount = data->getInt32();
        for(int i = 0; i < itemCount; i++) {
            std::string key;
            readId(data, key);
            DescriptorItem *item = readItem(data);
            if(item && item->isValid) {
                itemMap[key] = item;
            } else {
                // 解釈不能が出てきたら構造上スキップできないのでここで終了する
                if(item) {
                    delete item;
                }
                isValid = false;
                break;
            }
        }
        return isValid;
    }

    bool DescriptorReference::load(IteratorBase *data) {
        int itemCount = data->getInt32();
        for(int i = 0; i < itemCount; i++) {
            ReferenceItem *item = 0;
            type = (ReferenceType)data->getInt32();
            switch(type) {
                case 'prop':
                    item = new ReferenceProperty();
                    break;
                case 'Clss':
                    item = new ReferenceClass();
                    break;
                case 'Enmr':
                    item = new ReferenceEnumRef();
                    break;
                case 'rele':
                    item = new ReferenceOffset();
                    break;
                case 'Idnt':
                    item = new ReferenceIdentifier();
                    break;
                case 'indx':
                    item = new ReferenceIndex();
                    break;
                case 'name':
                    item = new ReferenceName();
                    break;
                default:
                    break;
            }
            if(item) {
                if(item->load(data)) {
                    items.push_back(item);
                } else {
                    delete item;
                }
            }
        }
        return isValid;
    }

    bool DescriptorList::load(IteratorBase *data) {
        int itemCount = data->getInt32();
        items.reserve(itemCount);
        for(int i = 0; i < itemCount; i++) {
            DescriptorItem *item = readItem(data);
            if(item && item->isValid) {
                items.push_back(item);
            } else {
                // 解釈不能が出てきたら構造上スキップできないのでここで終了する
                if(item->isValid) {
                    delete item;
                }
                isValid = false;
                break;
            }
        }
        return isValid;
    }

    bool DescriptorDouble::load(IteratorBase *data) {
        pun64 v;
        v.i = data->getInt64();
        val = v.f;
        return true;
    }

    bool DescriptorUnitFloat::load(IteratorBase *data) {
        unit = (DescriptorUnit)data->getInt32();
        pun64 v;
        v.i = data->getInt64();
        val = v.f;
        return true;
    }

    bool DescriptorString::load(IteratorBase *data) {
        data->getUnicodeString(val);
        return true;
    }

    bool DescriptorEnumerated::load(IteratorBase *data) {
        readId(data, typeId);
        readId(data, enumId);
        return true;
    }

    bool DescriptorInteger::load(IteratorBase *data) {
        val = data->getInt32();
        return true;
    }

    bool DescriptorBoolean::load(IteratorBase *data) {
        val = (data->getCh() != 0);
        return true;
    }

    bool DescriptorClass::load(IteratorBase *data) {
        data->getUnicodeString(name);
        readId(data, classId);
        return true;
    }

    bool DescriptorAlias::load(IteratorBase *data) {
        int size = data->getInt32();
        std::vector<char> buf(size + 1);
        data->getData(&buf[0], size);
        buf[size] = '\0';
        alias.assign(&buf[0]);
        return true;
    }

    // --------------------------------------------------------------------------
    // リファレンス
    // --------------------------------------------------------------------------

    bool ReferenceProperty::load(IteratorBase *data) {
        data->getUnicodeString(name);
        readId(data, classId);
        readId(data, keyId);
        return true;
    }

    bool ReferenceClass::load(IteratorBase *data) {
        data->getUnicodeString(name);
        readId(data, classId);
        return true;
    }

    bool ReferenceEnumRef::load(IteratorBase *data) {
        data->getUnicodeString(name);
        readId(data, classId);
        readId(data, typeId);
        readId(data, enumId);
        return true;
    }

    bool ReferenceOffset::load(IteratorBase *data) {
        data->getUnicodeString(name);
        readId(data, classId);
        offset = data->getInt32();
        return true;
    }

    bool ReferenceIdentifier::load(IteratorBase *data) {
        identifier = data->getInt32();
        return true;
    }

    bool ReferenceIndex::load(IteratorBase *data) {
        index = data->getInt32();
        return true;
    }

    bool ReferenceName::load(IteratorBase *data) {
        data->getUnicodeString(name);
        return true;
    }

} // namespace psd
