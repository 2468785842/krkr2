#include "psdfile.h"

#define USE_ZLIB
#ifdef USE_ZLIB

// vcpkg環境のzlibではオンにするとリンクエラーになる
// #define ZLIB_WINAPI

#define NOMINMAX
#include <zlib.h>
#endif

// #define ENABLE_BMP_OUTPUT
#ifdef ENABLE_BMP_OUTPUT
namespace psd {
    extern bool saveBmp(void *buffer, int width, int height, int size,
                        const char *filename);
}
#endif

namespace psd {

    static const float32_t OPAQ_F32 = 1.0f;
    static const uint32_t OPAQ_INT_BE = byteSwap32(*(uint32_t *)&(OPAQ_F32));

    // --------------------------------------------------------------------------
    // チャネルコンポーネントパック
    // --------------------------------------------------------------------------

    // コンポーネントカラーを32bitRGBAにパックする
    template <typename T>
    inline uint32_t rgbaCompoToRgba32(const ColorFormat &fmt, T r, T g, T b,
                                      T a) {
        // 値はBigEndian
#ifdef BOOST_LITTLE_ENDIAN
        return (
            uint32_t)(((a & 0xff) << fmt.aShift) | ((r & 0xff) << fmt.rShift) |
                      ((g & 0xff) << fmt.gShift) | ((b & 0xff) << fmt.bShift));
#else
        int cmpShift = (sizeof(T) - sizeof(uint8_t)) * 8;
        return (uint32_t)(((a >> cmpShift) << fmt.aShift) |
                          ((r >> cmpShift) << fmt.rShift) |
                          ((g >> cmpShift) << fmt.gShift) |
                          ((b >> cmpShift) << fmt.bShift));

#endif
    }

    // コンポーネントカラーを32bitRGBAにパックする(Aチャネル不透明)
    template <typename T>
    inline uint32_t rgbaCompoToRgba32(const ColorFormat &fmt, T r, T g, T b) {
        return rgbaCompoToRgba32<T>(fmt, r, g, b, (T)(-1));
    }

    // コンポーネントカラーを32bitRGBAにパックする
    //   32bit コンポーネントは浮動小数点なので特殊化で対応
    template <>
    inline uint32_t rgbaCompoToRgba32<uint32_t>(const ColorFormat &fmt,
                                                uint32_t _r, uint32_t _g,
                                                uint32_t _b, uint32_t _a) {
        pun32 r, g, b, a;
#ifdef BOOST_LITTLE_ENDIAN
        r.i = byteSwap32(_r);
        g.i = byteSwap32(_g);
        b.i = byteSwap32(_b);
        a.i = byteSwap32(_a);
#else
        r.i = _r;
        g.i = _g;
        b.i = _b;
        a.i = _a;
#endif
        return (uint32_t)(((uint32_t)(a.f * 255) << fmt.aShift) |
                          ((uint32_t)(r.f * 255) << fmt.rShift) |
                          ((uint32_t)(g.f * 255) << fmt.gShift) |
                          ((uint32_t)(b.f * 255) << fmt.bShift));
    }

    // コンポーネントカラーを32bitRGBAにパックする(Aチャネル不透明)
    //   32bit コンポーネントは浮動小数点なので特殊化で対応
    template <>
    inline uint32_t rgbaCompoToRgba32<uint32_t>(const ColorFormat &fmt,
                                                uint32_t r, uint32_t g,
                                                uint32_t b) {
        return rgbaCompoToRgba32<uint32_t>(fmt, r, g, b, OPAQ_INT_BE);
    }

    // グレーを32bitRGBAにパックする
    template <typename T>
    inline uint32_t grayToRgba32(const ColorFormat &fmt, T g, T a) {
#ifdef BOOST_LITTLE_ENDIAN
        uint8_t _g = g & 0xff;
        return (uint32_t)((a << fmt.aShift) | (_g << fmt.rShift) |
                          (_g << fmt.gShift) | (_g << fmt.bShift));
#else
        int cmpShift = (sizeof(T) - sizeof(uint8_t)) * 8;
        uint8_t _g = (g >> cmpShift) & 0xff;
        return (uint32_t)(((a >> cmpShift) << fmt.aShift) | (_g << fmt.rShift) |
                          (_g << fmt.gShift) | (_g << fmt.bShift));
#endif
    }

    // グレーを32bitRGBAにパックする(Aチャネル不透明)
    template <typename T>
    inline uint32_t grayToRgba32(const ColorFormat &fmt, T g) {
        return grayToRgba32<T>(fmt, g, (T)(-1));
    }

    // グレーを32bitRGBAにパックする
    //   32bit コンポーネントは浮動小数点なので特殊化で対応
    template <>
    inline uint32_t grayToRgba32<uint32_t>(const ColorFormat &fmt, uint32_t _g,
                                           uint32_t _a) {
        pun32 g, a;
#ifdef BOOST_LITTLE_ENDIAN
        g.i = byteSwap32(_g);
        a.i = byteSwap32(_a);
#else
        g.i = _g;
        a.i = _a;
#endif
        uint32_t gInt = (uint32_t)(g.f * 255);
        return (uint32_t)(((uint32_t)(a.f * 255) << fmt.aShift) |
                          (gInt << fmt.rShift) | (gInt << fmt.gShift) |
                          (gInt << fmt.bShift));
    }

    // グレーを32bitRGBAにパックする(Aチャネル不透明)
    //   32bit コンポーネントは浮動小数点なので特殊化で対応
    template <>
    inline uint32_t grayToRgba32<uint32_t>(const ColorFormat &fmt, uint32_t g) {
        return grayToRgba32<uint32_t>(fmt, g, OPAQ_INT_BE);
    }

    // CMYKを32bitRGBAにパックする
    template <typename T>
    inline uint32_t cmykCompoToRgba32(const ColorFormat &fmt, T _c, T _m, T _y,
                                      T _k, T _a) {
#ifdef BOOST_LITTLE_ENDIAN
        uint8_t c = 255 - _c & 0xff;
        uint8_t m = 255 - _m & 0xff;
        uint8_t y = 255 - _y & 0xff;
        uint8_t k = 255 - _k & 0xff;
        uint8_t a = _a & 0xff;
        uint8_t invK = _k & 0xff;
#else
        static const int f = (sizeof(T) - 1) * 8;
        uint8_t c = 255 - (_c >> f) & 0xff;
        uint8_t m = 255 - (_m >> f) & 0xff;
        uint8_t y = 255 - (_y >> f) & 0xff;
        uint8_t k = 255 - (_k >> f) & 0xff;
        uint8_t a = (_a >> f) & 0xff;
        uint8_t invK = (_k >> f) & 0xff;
#endif
        uint8_t r = (uint8_t)((invK * (255 - c)) >> 8);
        uint8_t g = (uint8_t)((invK * (255 - m)) >> 8);
        uint8_t b = (uint8_t)((invK * (255 - y)) >> 8);
        return rgbaCompoToRgba32<uint8_t>(fmt, r, g, b, a);
    }

    // CMYKを32bitRGBAにパックする(Aチャネル不透明)
    template <typename T>
    inline uint32_t cmykCompoToRgba32(const ColorFormat &fmt, T c, T m, T y,
                                      T k) {
        return cmykCompoToRgba32<T>(fmt, c, m, y, k, (T)(-1));
    }

    // MEMO CS5まででは32bit/chのCMYKデータは存在しない
    // CMYKを32bitRGBAにパックする
    //   32bit コンポーネントは浮動小数点なので特殊化で対応
    template <>
    inline uint32_t cmykCompoToRgba32<uint32_t>(const ColorFormat &fmt,
                                                uint32_t _c, uint32_t _m,
                                                uint32_t _y, uint32_t _k,
                                                uint32_t _a) {
        pun32 c, m, y, k, a;
#ifdef BOOST_LITTLE_ENDIAN
        c.i = byteSwap32(_c);
        m.i = byteSwap32(_m);
        y.i = byteSwap32(_y);
        k.i = byteSwap32(_k);
        a.i = byteSwap32(_a);
#else
        c.i = _c;
        m.i = _m;
        y.i = _y;
        k.i = _k;
        a.i = _a;
#endif
        float32_t invK = 1.0f - k.f;
        uint8_t r = (uint8_t)((invK * (1.0f - c.f)) * 255);
        uint8_t g = (uint8_t)((invK * (1.0f - m.f)) * 255);
        uint8_t b = (uint8_t)((invK * (1.0f - y.f)) * 255);
        return rgbaCompoToRgba32<uint8_t>(fmt, r, g, b, (uint8_t)(a.f * 255));
    }

    // CMYKを32bitRGBAにパックする(Aチャネル不透明)
    //   32bit コンポーネントは浮動小数点なので特殊化で対応
    template <>
    inline uint32_t cmykCompoToRgba32<uint32_t>(const ColorFormat &fmt,
                                                uint32_t c, uint32_t m,
                                                uint32_t y, uint32_t k) {
        return cmykCompoToRgba32<uint32_t>(fmt, c, m, y, k, OPAQ_INT_BE);
    }

    // --------------------------------------------------------------------------
    // チャネルマージ
    // --------------------------------------------------------------------------

    // 1bit bitmap を統合し pitch byte 単位で merged にフィルする
    void mergeChannelsBitmap(void *merged, int width, int height, uint8_t *src,
                             const ColorFormat &format, int bufPitchByte) {
        int pitchPixel = std::abs(bufPitchByte) / 4;
        int linePixel = std::min(width, pitchPixel);

        uint32_t c[2];
        c[0] = grayToRgba32<uint8_t>(format, 0xff, 0xff);
        c[1] = grayToRgba32<uint8_t>(format, 0x0, 0xff);

        int fullByte = linePixel / 8;
        int oddBits = linePixel % 8;
        for(int y = 0; y < height; y++) {
            uint8_t *bits = src + ((width + 7) / 8) * y;
            uint32_t *pix = (uint32_t *)((uint8_t *)merged + bufPitchByte * y);
            for(int x = 0; x < fullByte; x++) {
                *pix++ = c[*bits & (1 << 7)];
                *pix++ = c[*bits & (1 << 6)];
                *pix++ = c[*bits & (1 << 5)];
                *pix++ = c[*bits & (1 << 4)];
                *pix++ = c[*bits & (1 << 3)];
                *pix++ = c[*bits & (1 << 2)];
                *pix++ = c[*bits & (1 << 1)];
                *pix++ = c[*bits & (1 << 0)];
                bits++;
            }
            for(int x = 0; x < oddBits; x++) {
                *pix++ = c[*bits & (1 << (7 - x))];
            }
        }
    }

    // グレイチャネルを統合し pitch byte 単位で merged にフィルする
    template <typename T>
    void mergeChannelsGray(void *merged, int width, int height,
                           std::vector<int> &ids,
                           std::vector<uint8_t *> &decodedChannels,
                           const ColorFormat &format, int bufPitchByte) {
        T *chA = 0, *chG = 0;
        for(uint32_t i = 0; i < ids.size(); i++) {
            switch(ids[i]) {
                case CH_ID_TRANSP:
                    chA = (T *)decodedChannels[i];
                    break;
                case CH_ID_GRAY:
                    chG = (T *)decodedChannels[i];
                    break;
                default: /* TODO err */
                    break;
            }
        }

        int pitchPixel = std::abs(bufPitchByte) / 4;
        int linePixel = std::min(width, pitchPixel);
        if(chA) {
            for(int y = 0; y < height; y++) {
                uint32_t *pix =
                    (uint32_t *)((uint8_t *)merged + bufPitchByte * y);
                T *g = chG + width * y;
                T *a = chA + width * y;
                for(int x = 0; x < linePixel; x++) {
                    *pix++ = grayToRgba32<T>(format, *g++, *a++);
                }
            }
        } else {
            for(int y = 0; y < height; y++) {
                uint32_t *pix =
                    (uint32_t *)((uint8_t *)merged + bufPitchByte * y);
                T *g = chG + width * y;
                for(int x = 0; x < linePixel; x++) {
                    *pix++ = grayToRgba32<T>(format, *g++);
                }
            }
        }
    }

    // インデックスカラーを統合し pitch byte 単位で merged
    // にフィルする
    void mergeChannelsIndex(void *merged, int width, int height, uint8_t *src,
                            ColorTable &table, const ColorFormat &format,
                            int bufPitchByte) {
        int pitchPixel = std::abs(bufPitchByte) / 4;
        int linePixel = std::min(width, pitchPixel);

        for(int y = 0; y < height; y++) {
            uint8_t *index = src + width * y;
            uint32_t *pix = (uint32_t *)((uint8_t *)merged + bufPitchByte * y);
            for(int x = 0; x < linePixel; x++) {
                ColorRgba &c = table.colors[*index++];
                *pix++ = rgbaCompoToRgba32<uint8_t>(format, c.r, c.g, c.b, c.a);
            }
        }
    }

    // RGBチャネルを統合し pitch byte 単位で merged にフィルする
    template <typename T>
    void mergeChannelsRgb(void *merged, int width, int height,
                          std::vector<int> &ids,
                          std::vector<uint8_t *> &decodedChannels,
                          const ColorFormat &format, int bufPitchByte) {
        T *chR = 0, *chG = 0, *chB = 0, *chA = 0;
        for(uint32_t i = 0; i < ids.size(); i++) {
            switch(ids[i]) {
                case CH_ID_TRANSP:
                    chA = (T *)decodedChannels[i];
                    break;
                case CH_ID_RGB_R:
                    chR = (T *)decodedChannels[i];
                    break;
                case CH_ID_RGB_G:
                    chG = (T *)decodedChannels[i];
                    break;
                case CH_ID_RGB_B:
                    chB = (T *)decodedChannels[i];
                    break;
                default: /* TODO err */
                    break;
            }
        }

        int pitchPixel = std::abs(bufPitchByte) / 4;
        int linePixel = std::min(width, pitchPixel);
        if(chA) {
            for(int y = 0; y < height; y++) {
                uint32_t *pix =
                    (uint32_t *)((uint8_t *)merged + bufPitchByte * y);
                T *a = chA + width * y;
                T *r = chR + width * y;
                T *g = chG + width * y;
                T *b = chB + width * y;
                for(int x = 0; x < linePixel; x++) {
                    *pix++ =
                        rgbaCompoToRgba32<T>(format, *r++, *g++, *b++, *a++);
                }
            }
        } else {
            for(int y = 0; y < height; y++) {
                uint32_t *pix =
                    (uint32_t *)((uint8_t *)merged + bufPitchByte * y);
                T *r = chR + width * y;
                T *g = chG + width * y;
                T *b = chB + width * y;
                for(int x = 0; x < linePixel; x++) {
                    *pix++ = rgbaCompoToRgba32<T>(format, *r++, *g++, *b++);
                }
            }
        }
    }

    // CMYKチャネルを統合し pitch byte 単位で merged にフィルする
    template <typename T>
    void mergeChannelsCmyk(void *merged, int width, int height,
                           std::vector<int> &ids,
                           std::vector<uint8_t *> &decodedChannels,
                           const ColorFormat &format, int bufPitchByte) {
        T *chC = 0, *chM = 0, *chY = 0, *chK = 0, *chA = 0;
        for(uint32_t i = 0; i < ids.size(); i++) {
            switch(ids[i]) {
                case CH_ID_TRANSP:
                    chA = (T *)decodedChannels[i];
                    break;
                case CH_ID_CMYK_C:
                    chC = (T *)decodedChannels[i];
                    break;
                case CH_ID_CMYK_M:
                    chM = (T *)decodedChannels[i];
                    break;
                case CH_ID_CMYK_Y:
                    chY = (T *)decodedChannels[i];
                    break;
                case CH_ID_CMYK_K:
                    chK = (T *)decodedChannels[i];
                    break;
                default: /* TODO err */
                    break;
            }
        }

        int pitchPixel = std::abs(bufPitchByte) / 4;
        int linePixel = std::min(width, pitchPixel);
        if(chA) {
            for(int y = 0; y < height; y++) {
                uint32_t *pix =
                    (uint32_t *)((uint8_t *)merged + bufPitchByte * y);
                T *ap = chA + width * y;
                T *cp = chC + width * y;
                T *mp = chM + width * y;
                T *yp = chY + width * y;
                T *kp = chK + width * y;
                for(int x = 0; x < linePixel; x++) {
                    *pix++ = cmykCompoToRgba32<T>(format, *cp++, *mp++, *yp++,
                                                  *kp++, *ap++);
                }
            }
        } else {
            for(int y = 0; y < height; y++) {
                uint32_t *pix =
                    (uint32_t *)((uint8_t *)merged + bufPitchByte * y);
                T *cp = chC + width * y;
                T *mp = chM + width * y;
                T *yp = chY + width * y;
                T *kp = chK + width * y;
                for(int x = 0; x < linePixel; x++) {
                    *pix++ = cmykCompoToRgba32<T>(format, *cp++, *mp++, *yp++,
                                                  *kp++);
                }
            }
        }
    }

    // アルファチャネルにマスクチャネルをマージ
    template <typename T>
    void mergeMaskToAlpha(uint8_t *aCh, int al, int at, int ar, int ab,
                          uint8_t *mCh, int ml, int mt, int mr, int mb,
                          uint8_t defaultColor) {
        int aw = ar - al;
        int ah = ab - at;
        int mw = mr - ml;
        int mh = mb - mt;
        int aPitch = aw * sizeof(T);
        int mPitch = mw * sizeof(T);

        // マージ領域を処理
        // alphaとmaskの重なり領域
        int l = std::max(al, ml);
        int t = std::max(at, mt);
        int r = std::min(ar, mr);
        int b = std::min(ab, mb);
        int w = r - l;
        int h = b - t;
        if(w > 0 && h > 0) {
            // maskをalphaにマージ
            int aOffsetX = l - al;
            int aOffsetY = t - at;
            int mOffsetX = l - ml;
            int mOffsetY = t - mt;
            static float f = 1.0f / ((1LL << (sizeof(T) * 8)) - 1);
            for(int y = 0; y < h; y++) {
                T *ap = (T *)(aCh + (aOffsetY + y) * aPitch + aOffsetX);
                T *mp = (T *)(mCh + (mOffsetY + y) * mPitch + mOffsetX);
                T *out = ap;
                for(int x = 0; x < w; x++) {
                    *out++ = (T)(*ap++ * *mp++ * f);
                }
            }
        } else {
            // alphaとmaskの重なりがないのでここで終わり
            if(defaultColor == 0) {
                memset(aCh, 0x0, aw * ah * sizeof(T));
            }
            return;
        }

        // デフォルトカラーが0xffの場合は、マージ領域外は
        // αチャネルの値そのままになるので、何もしないで戻る
        if(defaultColor != 0) {
            return;
        }

        // マージ対象外の領域を処理
        // αチャネル領域をマージ範囲の座標で9個の矩形に分割
        struct rectangle {
            int w, h, l, t, r, b;
            rectangle() : w(0), h(0), l(0), t(0), r(0), b(0) {}
            void set(int _l, int _t, int _r, int _b) {
                l = _l;
                t = _t;
                r = _r;
                b = _b;
                w = r - l;
                h = b - t;
            }
            bool valid() { return (w > 0 && h > 0); }
            void invalidate() { w = h = 0; }
        } rect[3][3];

        // 座標をαチャネル領域相対に変換
        ml = std::max(0, ml - al);
        mt = std::max(0, mt - at);
        mr = std::min(aw, mr - al);
        mb = std::min(ah, mb - at);
        al = 0;
        at = 0;
        ar = aw;
        ab = ah;

        rect[0][0].set(al, at, ml, mt);
        rect[0][1].set(ml, at, mr, mt);
        rect[0][2].set(mr, at, ar, mt);
        rect[1][0].set(al, mt, ml, mb);
        rect[1][1].invalidate(); // マージ対象なのでinvalidのまま残して無視
        rect[1][2].set(mr, mt, ar, mb);
        rect[2][0].set(al, mb, ml, ab);
        rect[2][1].set(ml, mb, mr, ab);
        rect[2][2].set(mr, mb, ar, ab);

        // 同じ行の矩形領域を結合
        bool lineCombined[3] = { true, true, true };
        for(int y = 0; y < 3; y++) {
            for(int x = 1; x >= 0; x--) {
                if(rect[y][x].valid() && rect[y][x + 1].valid()) {
                    rect[y][x].r = rect[y][x + 1].r;
                    rect[y][x].w = rect[y][x].r - rect[y][x].l;
                    rect[y][x + 1].invalidate();
                } else {
                    lineCombined[y] = false;
                }
            }
        }

        // マージ対象以外をケア
        for(int y = 0; y < 3; y++) {
            if(lineCombined[y]) {
                // 行結合状態ならmemsetで領域一括セット
                rectangle &r = rect[y][0];
                uint8_t *out = aCh + r.t * aPitch;
                memset(out, 0x0, r.h * aPitch);
            } else {
                // 行結合済でなければ個別にライン単位処理
                for(int x = 0; x < 3; x++) {
                    rectangle &r = rect[y][x];
                    if(r.valid()) {
                        for(int l = 0; l < r.h; l++) {
                            uint8_t *out =
                                aCh + (r.t + l) * aPitch + r.l * sizeof(T);
                            memset(out, 0x0, r.w * sizeof(T));
                        }
                    }
                }
            }
        }
    }

    // --------------------------------------------------------------------------
    // 圧縮展開
    // --------------------------------------------------------------------------

    // RLE圧縮(PackBits)を展開する
    inline uint32_t decodePackBits(uint8_t *dst, uint8_t *src, int height,
                                   int channels = 1, int targetCh = 0,
                                   int lineDataOffset = 0) {
        uint32_t headerSize = sizeof(int16_t) * height * channels;
        int16_t *lineBytesBE = (int16_t *)src + height * targetCh;
        uint8_t *lineData = src + headerSize + lineDataOffset;
        uint8_t *decoded = dst;
        uint32_t readBytes = 0;

        for(int y = 0; y < height; y++, lineBytesBE++) {
#ifdef BOOST_LITTLE_ENDIAN
            int16_t lineBytes = byteSwap16(*lineBytesBE);
#else
            int16_t lineBytes = *lineBytesBE;
#endif
            for(int x = 0; x < lineBytes;) {
                uint8_t length = *lineData++;
                x++;
                if(length > 128) {
                    // ランレングス分同値コピー
                    length = (~length + 1) + 1;
                    memset(decoded, *lineData, length);
                    lineData++;
                    x++;
                } else if(length < 128) {
                    // ランがないのでベタコピー
                    length = length + 1;
                    memcpy(decoded, lineData, length);
                    lineData += length;
                    x += length;
                }
                decoded += length;
            }
            readBytes += lineBytes;
        }

        return readBytes;
    }

#ifdef USE_ZLIB
    // prediction なしのunzip展開
    bool decodeZipWithoutPrediction(void *dst, int dstSize, void *src,
                                    int srcSize) {
        z_stream zs;
        memset(&zs, 0, sizeof(z_stream));
        zs.zalloc = Z_NULL;
        zs.zfree = Z_NULL;
        zs.opaque = Z_NULL;
        zs.data_type = Z_BINARY;
        zs.next_in = (Bytef *)src;
        zs.avail_in = srcSize;
        zs.next_out = (Bytef *)dst;
        zs.avail_out = dstSize;

        int ret = inflateInit(&zs);
        if(ret != Z_OK) {
            return false;
        }

        do {
            ret = inflate(&zs, Z_NO_FLUSH);
            if(ret != Z_OK) {
                break;
            }
        } while(zs.avail_out != 0);

        inflateEnd(&zs);

        return (ret == Z_STREAM_END);
    }

    // prediction つき zip
    bool decodeZipWithPrediction(void *dst, int dstSize, void *src, int srcSize,
                                 int width, int height, int depth) {
        void *buf = 0;
        if(depth == 8 || depth == 16) {
            buf = dst;
        } else if(depth == 32) {
            buf = new uint8_t[dstSize];
        } else {
            return false; // err
        }
        bool success = decodeZipWithoutPrediction(buf, dstSize, src, srcSize);
        if(!success) {
            if(buf != dst) {
                delete[] buf;
            }
            return false;
        }

        // added
        if(depth == 8) {
            uint8_t *ptrData = (uint8_t *)buf;
            for(int i = 0; i < height; i++) {
                uint8_t *ptr = ptrData + i * width;
                uint8_t *ptrEnd = ptrData + (i + 1) * width;
                ptr++;
                while(ptr < ptrEnd) {
                    *ptr += *(ptr - 1);
                    ptr++;
                }
            }
        } else if(depth == 16) { // changed
            uint8_t *ptrData = (uint8_t *)buf;
            for(int i = 0; i < height; i++) {
#ifdef BOOST_LITTLE_ENDIAN
                // 後のチャンネルマージ時に big endian
                // として取り扱うので ここではバイトスワップせずに be
                // のままバイト単位の計算を行う
                uint8_t *ptr = ptrData + i * width * 2;
                for(int x = 0; x < (width - 1) * 2; x += 2) {
                    ptr[x + 2] += ptr[x + 0] + (ptr[x + 3] + ptr[x + 1]) / 256;
                    ptr[x + 3] += ptr[x + 1];
                }
#else
                uint16_t *ptr = (uint16_t *)(ptrData + i * width * 2);
                uint16_t *ptrEnd = (uint16_t *)(ptrData + (i + 1) * width * 2);
                ptr++;
                while(ptr < ptrEnd) {
                    *ptr += *(ptr - 1);
                    ptr++;
                }
#endif
            }
        } else if(depth == 32) {
            // 4バイトをバイト毎に並べてあるのでバイト単位でデルタ復元
            uint8_t *ptrData = (uint8_t *)buf;
            for(int i = 0; i < height; i++) {
                uint8_t *ptr = ptrData + i * width * 4;
                uint8_t *ptrEnd = ptrData + (i + 1) * width * 4;
                ptr++;
                while(ptr < ptrEnd) {
                    *ptr = (uint8_t)(*ptr + *(ptr - 1));
                    ptr++;
                }
            }

            // バイト単位で復元したものを4バイト値にパック
            int offset1 = width;
            int offset2 = 2 * offset1;
            int offset3 = 3 * offset1;
            for(int i = 0; i < height; i++) {
                uint8_t *dstPtr = (uint8_t *)dst + i * width * 4;
                uint8_t *dstPtrEnd = (uint8_t *)dst + (i + 1) * width * 4;
                uint8_t *srcPtr = ptrData + i * width * 4;
                while(dstPtr < dstPtrEnd) {
                    *dstPtr++ = *(srcPtr);
                    *dstPtr++ = *(srcPtr + offset1);
                    *dstPtr++ = *(srcPtr + offset2);
                    *dstPtr++ = *(srcPtr + offset3);
                    srcPtr++;
                }
            }
            delete[] buf;
        }
        return true;
    }
#endif // USE_ZLIB

    // --------------------------------------------------------------------------
    // 画像取得
    // --------------------------------------------------------------------------
    // レイヤー画像を取得
    bool PSDFile::getLayerImageById(int layerId, void *buf,
                                    const ColorFormat &format, int bufPitchByte,
                                    ImageMode mode) {
        LayerInfo *layer = getLayerById(layerId);
        if(layer) {
            return getLayerImage(*layer, buf, format, bufPitchByte, mode);
        } else {
            return false;
        }
    }

    // レイヤー画像を取得
    bool PSDFile::getLayerImage(LayerInfo &layer, void *buf,
                                const ColorFormat &format, int bufPitchByte,
                                ImageMode mode) {
        psd::LayerMask &mask = layer.extraData.layerMask;

        int imageWidth = layer.width;
        int imageHeight = layer.height;
        int imagePixels = imageWidth * imageHeight;
        int imageChannelBytes = 0;

        int maskWidth = mask.width;
        int maskHeight = mask.height;
        int maskPixels = maskWidth * maskHeight;
        int maskChannelBytes = 0;

        if(bufPitchByte == 0) {
            bufPitchByte = imageWidth * 4;
        }

        switch(header.depth) {
            case 1:
                imageChannelBytes = (imageWidth + 7) / 8 * imageHeight;
                maskChannelBytes = 0;
                break;
            case 8:
                imageChannelBytes = imagePixels;
                maskChannelBytes = maskPixels;
                break;
            case 16:
                imageChannelBytes = imagePixels * 2;
                maskChannelBytes = maskPixels * 2;
                break;
            case 32:
                imageChannelBytes = imagePixels * 4;
                maskChannelBytes = maskPixels * 4;
                break;
            default:
                return false;
                break;
        }

        uint8_t *tmpSourceBuffer = 0;
        int tmpSourceBufferSize = 0;
        int channels = (int)layer.channels.size();
        std::vector<uint8_t *> decodedChannels; // (channels);
        std::vector<int> channelIds; // (channels);

        dprint("layer: %s (%d ch)\n", layer.extraData.layerName.c_str(),
               channels);
        dprint(" image: (%d x %d) %d pixels, %d bytes/ch\n", imageWidth,
               imageHeight, imagePixels, imageChannelBytes);
        dprint(" mask:  (%d x %d) %d pixels, %d bytes/ch\n", maskWidth,
               maskHeight, maskPixels, maskChannelBytes);

        int alphaChannelIndex = -1;
        int maskChannelIndex = -1;
        for(int i = 0; i < channels; i++) {
            ChannelInfo &channel = layer.channels[i];
            if(channel.imageData == 0) {
                continue;
            }

            // 分離取得モードでは、それぞれ不要なチャネルは処理しない
            if((mode == IMAGE_MODE_IMAGE && channel.isMaskChannel()) ||
               (mode == IMAGE_MODE_MASK && !channel.isMaskChannel())) {
                continue;
            }

            // ソースチャネルバッファの準備
            channel.imageData->init();
            int compressionId = channel.imageData->getInt16();
            int dataLength =
                channel.length - 2; // 2: 頭についてる compress id 分減らす
            if(compressionId != 0) {
                // 圧縮の場合は一時ソースバッファにコピーしておく
                if(dataLength > tmpSourceBufferSize) {
                    delete[] tmpSourceBuffer;
                    tmpSourceBuffer = new uint8_t[dataLength];
                    tmpSourceBufferSize = dataLength;
                }
                channel.imageData->getData(tmpSourceBuffer, dataLength);
            }

            // 展開先チャネルバッファ&チャネルidのセット
            int bufSize =
                channel.isMaskChannel() ? maskChannelBytes : imageChannelBytes;
            uint8_t *decodedChannel = new uint8_t[bufSize];
            uint8_t channelId = channel.id;

            int width = channel.isMaskChannel() ? maskWidth : imageWidth;
            int height = channel.isMaskChannel() ? maskHeight : imageHeight;

            // チャネルデータ展開
            switch(compressionId) {
                case 0: // raw
                    channel.imageData->getData(decodedChannel, bufSize);
                    break;
                case 1: // RLE(PackBits)
                    decodePackBits(decodedChannel, tmpSourceBuffer, height);
                    break;
                case 2: // zip (w/o prediction)
#ifdef USE_ZLIB
                    decodeZipWithoutPrediction(decodedChannel, bufSize,
                                               tmpSourceBuffer, dataLength);
#else
                    memset(decodedChannel, 0xff, bufSize);
#endif
                    break;
                case 3: // zip (w/ prediction)
#ifdef USE_ZLIB
                    decodeZipWithPrediction(decodedChannel, bufSize,
                                            tmpSourceBuffer, dataLength, width,
                                            height, header.depth);
#else
                    memset(decodedChannel, 0xff, bufSize);
#endif
                    break;
                default:
                    memset(decodedChannel, 0xff, bufSize);
                    break;
            }

            // TODO real user mask と user mask が同時に入ってるケース
            switch(channel.id) {
                case CH_ID_TRANSP:
                    alphaChannelIndex = i;
                    break;
                case CH_ID_UMASK:
                    maskChannelIndex = i;
                    break;
                case CH_ID_REAL_UMASK:
                    maskChannelIndex = i;
                    break;
                default:
                    break;
            }

            decodedChannels.push_back(decodedChannel);
            channelIds.push_back(channel.id);
        }

        if(tmpSourceBuffer) {
            delete[] tmpSourceBuffer;
        }

        if(mode == IMAGE_MODE_MASK && maskChannelIndex < 0) {
            return false;
        }

        // イメージ取得モードに合わせてデータを調整
        int colorMode = header.mode;
        switch(mode) {
            case IMAGE_MODE_IMAGE:
                // イメージのみ。とくに調整は不要
                break;
            case IMAGE_MODE_MASKEDIMAGE:
                // マスクチャネルをアルファチャネルに繰り込む
                if(maskChannelIndex >= 0 && alphaChannelIndex >= 0) {
                    uint8_t *maskChannel = decodedChannels[maskChannelIndex];
                    uint8_t *alphaChannel = decodedChannels[alphaChannelIndex];
                    switch(header.depth) {
                        case 8:
                            mergeMaskToAlpha<uint8_t>(
                                alphaChannel, layer.left, layer.top,
                                layer.right, layer.bottom, maskChannel,
                                mask.left, mask.top, mask.right, mask.bottom,
                                mask.defaultColor);
                            break;
                        case 16:
                            mergeMaskToAlpha<uint16_t>(
                                alphaChannel, layer.left, layer.top,
                                layer.right, layer.bottom, maskChannel,
                                mask.left, mask.top, mask.right, mask.bottom,
                                mask.defaultColor);
                            break;
                        case 32:
                            mergeMaskToAlpha<uint32_t>(
                                alphaChannel, layer.left, layer.top,
                                layer.right, layer.bottom, maskChannel,
                                mask.left, mask.top, mask.right, mask.bottom,
                                mask.defaultColor);
                            break;
                        default:
                            break;
                    }
                }
                break;
            case IMAGE_MODE_MASK:
                // maskモードのときはグレー画像にフェイクする
                colorMode = COLOR_MODE_GRAYSCALE;
                imageWidth = maskWidth;
                imageHeight = maskHeight;
                channelIds[0] = CH_ID_GRAY;
                break;
            default:
                break;
        }

        // チャネルデータをピクセルデータにマージ
        switch(colorMode) {
            case COLOR_MODE_BITMAP:
                mergeChannelsBitmap(buf, imageWidth, imageHeight,
                                    decodedChannels[0], format, bufPitchByte);
                break;
            case COLOR_MODE_GRAYSCALE:
                switch(header.depth) {
                    case 8:
                        mergeChannelsGray<uint8_t>(buf, imageWidth, imageHeight,
                                                   channelIds, decodedChannels,
                                                   format, bufPitchByte);
                        break;
                    case 16:
                        mergeChannelsGray<uint16_t>(
                            buf, imageWidth, imageHeight, channelIds,
                            decodedChannels, format, bufPitchByte);
                        break;
                    case 32:
                        mergeChannelsGray<uint32_t>(
                            buf, imageWidth, imageHeight, channelIds,
                            decodedChannels, format, bufPitchByte);
                        break;
                    default:
                        break;
                }
                break;
            case COLOR_MODE_RGB:
                switch(header.depth) {
                    case 8:
                        mergeChannelsRgb<uint8_t>(buf, imageWidth, imageHeight,
                                                  channelIds, decodedChannels,
                                                  format, bufPitchByte);
                        break;
                    case 16:
                        mergeChannelsRgb<uint16_t>(buf, imageWidth, imageHeight,
                                                   channelIds, decodedChannels,
                                                   format, bufPitchByte);
                        break;
                    case 32:
                        mergeChannelsRgb<uint32_t>(buf, imageWidth, imageHeight,
                                                   channelIds, decodedChannels,
                                                   format, bufPitchByte);
                        break;
                    default:
                        break;
                }
                break;
            case COLOR_MODE_INDEXED:
                mergeChannelsIndex(buf, imageWidth, imageHeight,
                                   decodedChannels[0], colorTable, format,
                                   bufPitchByte);
                break;
            case COLOR_MODE_CMYK:
                switch(header.depth) {
                    case 8:
                        mergeChannelsCmyk<uint8_t>(buf, imageWidth, imageHeight,
                                                   channelIds, decodedChannels,
                                                   format, bufPitchByte);
                        break;
                    case 16:
                        mergeChannelsCmyk<uint16_t>(
                            buf, imageWidth, imageHeight, channelIds,
                            decodedChannels, format, bufPitchByte);
                        break;
                    case 32:
                        mergeChannelsCmyk<uint32_t>(
                            buf, imageWidth, imageHeight, channelIds,
                            decodedChannels, format, bufPitchByte);
                        break;
                    default:
                        break;
                }
                break;
            case COLOR_MODE_MULTICHANNEL:
            case COLOR_MODE_DUOTONE:
            case COLOR_MODE_LAB:
                // unsuported yet
                break;
            default:
                break;
        }

        for(int i = 0; i < (int)decodedChannels.size(); i++) {
            delete[] decodedChannels[i];
        }

#ifdef ENABLE_BMP_OUTPUT
        char name[256];
        sprintf(name, "layer_%s.bmp", layer.extraData.layerName.c_str());
        saveBmp(buf, imageWidth, imageHeight, imagePixels * 4, name);
#endif

        return true;
    }

    bool PSDFile::getMergedImage(void *buf, const ColorFormat &format,
                                 int bufPitchByte) {
        int imageWidth = header.width;
        int imageHeight = header.height;
        int imagePixels = imageWidth * imageHeight;
        int imageChannelBytes = 0;
        switch(header.depth) {
            case 1:
                imageChannelBytes = (imageWidth + 7) / 8 * imageHeight;
                break;
            case 8:
                imageChannelBytes = imagePixels;
                break;
            case 16:
                imageChannelBytes = imagePixels * 2;
                break;
            case 32:
                imageChannelBytes = imagePixels * 4;
                break;
            default:
                return false;
                break;
        }
        if(bufPitchByte == 0) {
            bufPitchByte = imageWidth * 4;
        }

        // ソースチャネルバッファの準備
        imageData->init();
        uint8_t *tmpSourceBuffer = 0;
        int compressionId = imageData->getInt16();
        int dataLength = imageData->rest();
        if(compressionId != 0) {
            tmpSourceBuffer = new uint8_t[dataLength];
            imageData->getData(tmpSourceBuffer, dataLength);
        }

        // 展開先チャネルバッファの準備
        int channels = header.channels;
        std::vector<uint8_t *> decodedChannels(channels);
        std::vector<int> channelIds(channels);
        for(int i = 0; i < channels; i++) {
            decodedChannels[i] = new uint8_t[imageChannelBytes];
            channelIds[i] = i;
        }

        // チャネルデータ展開
        switch(compressionId) {
            case 0: // raw
            {
                for(int i = 0; i < channels; i++) {
                    imageData->getData(decodedChannels[i], imageChannelBytes);
                }
            } break;
            case 1: // RLE(PackBits)
            {
                uint32_t nextOffset = 0;
                for(int i = 0; i < channels; i++) {
                    nextOffset +=
                        decodePackBits(decodedChannels[i], tmpSourceBuffer,
                                       imageHeight, channels, i, nextOffset);
                }
            } break;
            case 2: // zip (w/o prediction)
            {
                for(int i = 0; i < channels; i++) {
#ifdef USE_ZLIB
                    decodeZipWithoutPrediction(decodedChannels[i],
                                               imageChannelBytes,
                                               tmpSourceBuffer, dataLength);
#else
                    memset(decodedChannel, 0xff, bufSize);
#endif
                }
            } break;
            case 3: // zip (w/ prediction)
            {
                for(int i = 0; i < channels; i++) {
#ifdef USE_ZLIB
                    decodeZipWithPrediction(
                        decodedChannels[i], imageChannelBytes, tmpSourceBuffer,
                        dataLength, imageWidth, imageHeight, header.depth);
#else
                    memset(decodedChannel, 0xff, bufSize);
#endif
                }
            } break;
            default:
                break;
        }

        if(tmpSourceBuffer) {
            delete[] tmpSourceBuffer;
        }

        switch(header.mode) {
            case COLOR_MODE_BITMAP:
                mergeChannelsBitmap(buf, imageWidth, imageHeight,
                                    decodedChannels[0], format, bufPitchByte);
                break;
            case COLOR_MODE_GRAYSCALE:
                switch(header.depth) {
                    case 8:
                        mergeChannelsGray<uint8_t>(buf, imageWidth, imageHeight,
                                                   channelIds, decodedChannels,
                                                   format, bufPitchByte);
                        break;
                    case 16:
                        mergeChannelsGray<uint16_t>(
                            buf, imageWidth, imageHeight, channelIds,
                            decodedChannels, format, bufPitchByte);
                        break;
                    case 32:
                        mergeChannelsGray<uint32_t>(
                            buf, imageWidth, imageHeight, channelIds,
                            decodedChannels, format, bufPitchByte);
                        break;
                    default:
                        break;
                }
                break;
            case COLOR_MODE_RGB:
                if(channelIds.size() > 3) {
                    channelIds[3] = CH_ID_TRANSP;
                }
                switch(header.depth) {
                    case 8:
                        mergeChannelsRgb<uint8_t>(buf, imageWidth, imageHeight,
                                                  channelIds, decodedChannels,
                                                  format, bufPitchByte);
                        break;
                    case 16:
                        mergeChannelsRgb<uint16_t>(buf, imageWidth, imageHeight,
                                                   channelIds, decodedChannels,
                                                   format, bufPitchByte);
                        break;
                    case 32:
                        mergeChannelsRgb<uint32_t>(buf, imageWidth, imageHeight,
                                                   channelIds, decodedChannels,
                                                   format, bufPitchByte);
                        break;
                    default:
                        break;
                }
                break;
            case COLOR_MODE_INDEXED:
                mergeChannelsIndex(buf, imageWidth, imageHeight,
                                   decodedChannels[0], colorTable, format,
                                   bufPitchByte);
                break;
            case COLOR_MODE_CMYK:
                switch(header.depth) {
                    case 8:
                        mergeChannelsCmyk<uint8_t>(buf, imageWidth, imageHeight,
                                                   channelIds, decodedChannels,
                                                   format, bufPitchByte);
                        break;
                    case 16:
                        mergeChannelsCmyk<uint16_t>(
                            buf, imageWidth, imageHeight, channelIds,
                            decodedChannels, format, bufPitchByte);
                        break;
                    case 32:
                        mergeChannelsCmyk<uint32_t>(
                            buf, imageWidth, imageHeight, channelIds,
                            decodedChannels, format, bufPitchByte);
                        break;
                    default:
                        break;
                }
                break;
            case COLOR_MODE_MULTICHANNEL:
            case COLOR_MODE_DUOTONE:
            case COLOR_MODE_LAB:
                // unsuported yet
                break;
            default:
                break;
        }

        for(int i = 0; i < channels; i++) {
            delete[] decodedChannels[i];
        }

#ifdef ENABLE_BMP_OUTPUT
        saveBmp(buf, imageWidth, imageHeight, imagePixels * 4, "merged.bmp");
#endif

        return true;
    }

} // namespace psd
