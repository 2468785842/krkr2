//---------------------------------------------------------------------------
// TVP message formatting and exception helpers
//---------------------------------------------------------------------------
#ifndef TVP_ERROR_H_
#define TVP_ERROR_H_

#include "tjs.h"

TJS_EXP_FUNC_DEF(ttstr, TVPFormatMessage,
                 (const tjs_char *msg, const ttstr &p1));

TJS_EXP_FUNC_DEF(ttstr, TVPFormatMessage,
                 (const tjs_char *msg, const ttstr &p1, const ttstr &p2));

TJS_EXP_FUNC_DEF(void, TVPThrowExceptionMessage, (const tjs_char *msg));

TJS_EXP_FUNC_DEF(void, TVPThrowExceptionMessage,
                 (const tjs_char *msg, const ttstr &p1, tjs_int num));

TJS_EXP_FUNC_DEF(void, TVPThrowExceptionMessage,
                 (const tjs_char *msg, const ttstr &p1));

TJS_EXP_FUNC_DEF(void, TVPThrowExceptionMessage,
                 (const tjs_char *msg, const ttstr &p1, const ttstr &p2));

#endif
