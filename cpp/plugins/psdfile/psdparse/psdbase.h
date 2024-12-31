#ifndef __psdbase_h__
#define __psdbase_h__

#include <string>

#ifdef _DEBUG
#define dprint(...) printf(__VA_ARGS__);
#else
#define dprint(...) ((void)0)
#endif

namespace psd {
#ifdef _MSC_VER
#ifndef _STDINT
  typedef __int8 int8_t;
  typedef unsigned __int8 uint8_t;
  typedef __int16 int16_t;
  typedef unsigned __int16 uint16_t;
  typedef __int32 int32_t;
  typedef unsigned __int32 uint32_t;
  typedef __int64 int64_t;
  typedef unsigned __int64 uint64_t;
#endif
#else
  #include <stdint.h>
#endif

  typedef float  float32_t;
  typedef double float64_t;

  // ビット情報を維持した型変換用のunion
  union pun32 {
    uint32_t  i;
    float32_t f;
  };
  union pun64 {
    uint64_t  i;
    float64_t f;
  };

  // イメージ取得時にカラー配置を指定するためのフォーマット構造体
  struct ColorFormat {
    ColorFormat(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    : rShift(r), gShift(g), bShift(b), aShift(a) {}

    uint8_t rShift;
    uint8_t gShift;
    uint8_t bShift;
    uint8_t aShift;
  };

  // デフォルトのWindows向けBGRA(LittleEndian)フォーマット
  static const ColorFormat BGRA_LE(16, 8, 0, 24);

  // イテレータ参照用基底クラス
	class IteratorBase {
	public:
		IteratorBase() {};
		virtual ~IteratorBase() {};
		virtual IteratorBase *clone() = 0;
    virtual IteratorBase *cloneOffset(int offset) = 0;
		virtual void init() = 0;
		virtual int getCh() = 0;
		virtual int16_t getInt16(bool convToNative=true) = 0;
		virtual int32_t getInt32(bool convToNative=true) = 0;
		virtual int64_t getInt64(bool convToNative=true) = 0;
		virtual int getData(void *buffer, int size) = 0;
		virtual bool eoi() = 0;
    virtual void getUnicodeString(std::wstring &str, bool convToNative=true) = 0;
    virtual int size() = 0;
    virtual int rest() = 0;
    virtual void advance(int size) = 0;
	};

  // swap utilities
  inline uint16_t byteSwap16(uint16_t x) {
    return (((x & 0xff00) >> 8) | ((x & 0x00ff) << 8));
  }

  inline uint32_t byteSwap32(uint32_t x) {
    return ((((x) & 0xff000000) >> 24) |
            (((x) & 0x00ff0000) >>  8) |
            (((x) & 0x0000ff00) <<  8) |
            (((x) & 0x000000ff) << 24));
  }

  inline uint64_t byteSwap64(uint64_t x) {
    return ((x >> 56) |
            ((x >> 40) & 0xff00) |
            ((x >> 24) & 0xff0000) |
            ((x >>  8) & 0xff000000) |
            ((x <<  8) & (0xffull << 32)) |
            ((x << 24) & (0xffull << 40)) |
            ((x << 40) & (0xffull << 48)) |
            (x << 56));
  }
}

#endif // __psdbase_h__
