
#include "tjsCommHead.h"
#include "CharacterSet.h"

//---------------------------------------------------------------------------
bool TVPEncodeUTF8ToUTF16(ttstr &output, const std::string &source) {
    tjs_int len = TVPUtf8ToWideCharString(source.c_str(), nullptr);
    if(len == -1)
        return false;
    std::vector<tjs_char> outbuf(len + 1, 0);
    tjs_int ret = TVPUtf8ToWideCharString(source.c_str(), &(outbuf[0]));
    if(ret) {
        outbuf[ret] = L'\0';
        output = &(outbuf[0]);
        return true;
    }
    return false;
}
