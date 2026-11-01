#include "ArchiveFilename.h"

#include "MsgIntf.h"
#include <cstring>
#include <limits>

extern int TVPShowSimpleMessageBox(const ttstr &text, const ttstr &caption);

//---------------------------------------------------------------------------
void storeFilename(ttstr &name, const char *narrowName, const ttstr &filename) {
    // 长度查询遇到非法序列只返回前缀长度，不能用它验证 UTF-8。
    const size_t capacity = std::strlen(narrowName);
    if(capacity > std::numeric_limits<tjs_int>::max())
        TVPThrowExceptionMessage(TVPReadError);
    ttstr converted;
    tjs_char *p = converted.AllocBuffer(static_cast<tjs_uint>(capacity));
    const size_t len = TJS_narrowtowide(p, narrowName, capacity);
    if(len == static_cast<size_t>(-1)) {
        ttstr msg("Filename is not encoded in UTF8 in archive:\n");
        TVPShowSimpleMessageBox(msg + filename, TJS_W("Error"));
        TVPThrowExceptionMessage(TJS_W("Invalid archive entry name"));
    }
    p[len] = 0;
    converted.FixLen();
    name = converted;
}
