//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// string heap management used by tTJSVariant and tTJSString
//---------------------------------------------------------------------------
#ifndef tjsVariantStringH
#define tjsVariantStringH

#include "tjsConfig.h"
#include <cstdlib>
#include <cstring>
#include <atomic>

namespace TJS {
    class tTJSVariant;
// #define TJS_DEBUG_UNRELEASED_STRING
// #define TJS_DEBUG_CHECK_STRING_HEAP_INTEGRITY
// #define TJS_DEBUG_DUMP_STRING

/*[*/
//---------------------------------------------------------------------------
// tTJSVariantString stuff
//---------------------------------------------------------------------------
#define TJS_VS_SHORT_LEN 21

    /*]*/
    class tTJSVariantString;

    extern tjs_int TJSGetShorterStrLen(const tjs_char *str, ssize_t max);

    extern tTJSVariantString *TJSAllocStringHeap();

    extern void TJSDeallocStringHeap(tTJSVariantString *vs);

    extern void TJSThrowStringAllocError();

    extern void TJSThrowNarrowToWideConversionError();

    extern void TJSCompactStringHeap();

    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    // base memory allocation functions for long string
    //---------------------------------------------------------------------------
    TJS_EXP_FUNC_DEF(tjs_char *, TJSVS_malloc, (tjs_uint len));

    TJS_EXP_FUNC_DEF(tjs_char *, TJSVS_realloc, (tjs_char * buf, tjs_uint len));

    TJS_EXP_FUNC_DEF(void, TJSVS_free, (tjs_char * buf));

    struct tTJSVariantString_S {
        std::atomic_long RefCount{};
        tjs_char *LongString{};
        tjs_char ShortString[TJS_VS_SHORT_LEN + 1]{};
        tjs_int Length{}; // string length
        tjs_uint32 HeapFlag{};
        tjs_uint32 Hint{};
    };

    /*start-of-tTJSVariantString*/
    class tTJSVariantString : public tTJSVariantString_S {
    public:
        tTJSVariantString() = default;
        void AddRef() { this->RefCount++; }

        void Release();

        void SetString(const tjs_char *ref, ssize_t maxlen = -1) {
            if(LongString)
                TJSVS_free(LongString), LongString = nullptr;
            tjs_int len;
            if(maxlen != -1)
                len = TJSGetShorterStrLen(ref, maxlen);
            else
                len = (tjs_int)TJS_strlen(ref);

            Length = len;
            if(len > TJS_VS_SHORT_LEN) {
                LongString = TJSVS_malloc(len + 1);
                TJS_strcpy_maxlen(LongString, ref, len);
            } else {
                TJS_strcpy_maxlen(ShortString, ref, len);
            }
        }

        void SetString(const tjs_nchar *ref) {
            if(LongString)
                TJSVS_free(LongString), LongString = nullptr;
            tjs_int len = (tjs_int)TJS_narrowtowidelen(ref);
            if(len == -1)
                TJSThrowNarrowToWideConversionError();

            Length = len;
            if(len > TJS_VS_SHORT_LEN) {
                LongString = TJSVS_malloc(len + 1);
                LongString[TJS_narrowtowide(LongString, ref, len)] = 0;
            } else {
                ShortString[TJS_narrowtowide(ShortString, ref,
                                             TJS_VS_SHORT_LEN)] = 0;
            }
        }

        void AllocBuffer(tjs_uint len) {
            /* note that you must call FixLength if you allocate
               larger than the actual string size */

            if(LongString)
                TJSVS_free(LongString), LongString = nullptr;

            Length = len;
            if(len > TJS_VS_SHORT_LEN) {
                LongString = TJSVS_malloc(len + 1);
                LongString[len] = 0;
            } else {
                ShortString[len] = 0;
            }
        }

        void ResetString(const tjs_char *ref) {
            if(LongString)
                TJSVS_free(LongString), LongString = nullptr;
            SetString(ref);
        }

        void AppendBuffer(tjs_uint applen) {
            /* note that you must call FixLength if you allocate
               larger than the actual string size */

            // assume this != nullptr
            tjs_int newlen = Length += applen;
            if(LongString) {
                // still long string
                LongString = TJSVS_realloc(LongString, newlen + 1);
                LongString[newlen] = 0;
                return;
            } else {
                if(newlen <= TJS_VS_SHORT_LEN) {
                    // still short string
                    ShortString[newlen] = 0;
                    return;
                }
                // becomes a long string
                tjs_char *newbuf = TJSVS_malloc(newlen + 1);
                TJS_strcpy(newbuf, ShortString);
                LongString = newbuf;
                LongString[newlen] = 0;
                return;
            }
        }

        void Append(const tjs_char *str) {
            // assume this != nullptr
            Append(str, (tjs_int)TJS_strlen(str));
        }

        void Append(const tjs_char *str, tjs_int applen) {
            // assume this != nullptr
            tjs_int orglen = Length;
            tjs_int newlen = Length += applen;
            if(LongString) {
                // still long string
                LongString = TJSVS_realloc(LongString, newlen + 1);
                TJS_strcpy(LongString + orglen, str);
                return;
            } else {
                if(newlen <= TJS_VS_SHORT_LEN) {
                    // still short string
                    TJS_strcpy(ShortString + orglen, str);
                    return;
                }
                // becomes a long string
                tjs_char *newbuf = TJSVS_malloc(newlen + 1);
                TJS_strcpy(newbuf, ShortString);
                TJS_strcpy(newbuf + orglen, str);
                LongString = newbuf;
                return;
            }
        }

        TJS_CONST_METHOD_DEF(
            TJS_METHOD_RET(const tjs_char *), operator const tjs_char *, ());

        tjs_int GetLength() const;

        tTJSVariantString *FixLength();

        tjs_uint32 *GetHint() { return &Hint; }

        tTVInteger ToInteger() const;

        tTVReal ToReal() const;

        void ToNumber(tTJSVariant &dest) const;

        tjs_int GetRefCount() const { return this->RefCount; }

        tjs_int QueryPersistSize() const {
            return sizeof(tjs_uint) + GetLength() * sizeof(tjs_char);
        }

        void Persist(tjs_uint8 *dest) const {
            tjs_uint size;
            const tjs_char *ptr = LongString ? LongString : ShortString;
            *(tjs_uint *)dest = size = GetLength();
            dest += sizeof(tjs_uint);
            while(size--) {
                *(tjs_char *)dest = *ptr;
                dest += sizeof(tjs_char);
                ptr++;
            }
        }
    };
    TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSAllocVariantString,
                     (const tjs_char *ref1, const tjs_char *ref2));

    tTJSVariantString *TJSAllocVariantString(const tjs_char *ref, size_t n);

    TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSAllocVariantString,
                     (const tjs_char *ref));

    TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSAllocVariantString,
                     (const tjs_nchar *ref));

    TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSAllocVariantString,
                     (const tjs_uint8 **src));

    TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSAllocVariantStringBuffer,
                     (tjs_uint len));

    TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSAppendVariantString,
                     (tTJSVariantString * str, const tjs_char *app));

    TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSAppendVariantString,
                     (tTJSVariantString * str, const tTJSVariantString *app));

    TJS_EXP_FUNC_DEF(tTJSVariantString *, TJSFormatString,
                     (const tjs_char *format, tjs_uint numparams,
                      tTJSVariant **params));

    //---------------------------------------------------------------------------
} // namespace TJS
#endif
