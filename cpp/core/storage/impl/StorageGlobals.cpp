#include "tjsCommHead.h"

#include "MsgIntf.h"
#include "StorageIntf.h"

//---------------------------------------------------------------------------
// 归档/存储共用全局
//---------------------------------------------------------------------------
tjs_char TVPArchiveDelimiter = '>';

//---------------------------------------------------------------------------
ttstr TVPStringFromBMPUnicode(const tjs_uint16 *src, tjs_int maxlen) {
    if(sizeof(tjs_char) == 2) {
        if(maxlen == -1)
            return ttstr(reinterpret_cast<const tjs_char *>(src));
        return ttstr(reinterpret_cast<const tjs_char *>(src), maxlen);
    }
    if(sizeof(tjs_char) == 4) {
        tjs_int len = 0;
        const tjs_uint16 *p = src;
        while(*p)
            len++, p++;
        if(maxlen != -1 && len > maxlen)
            len = maxlen;
        ttstr ret(static_cast<tTJSStringBufferLength>(len));
        tjs_char *dest = ret.Independ();
        p = src;
        while(len && *p) {
            *dest = *p;
            dest++;
            p++;
            len--;
        }
        *dest = 0;
        ret.FixLen();
        return ret;
    }
    TVPThrowExceptionMessage(TVPTjsCharMustBeTwoOrFour);
    return ttstr();
}
