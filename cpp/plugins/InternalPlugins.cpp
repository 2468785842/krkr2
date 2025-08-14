#include "ncbind/ncbind.hpp"
#include <algorithm>

void TVPLoadInternalPlugins() {
    ncbAutoRegister::AllRegist();
    ncbAutoRegister::LoadModule(TJS_W("xp3filter.dll"));
}

[[maybe_unused]] void TVPUnloadInternalPlugins() {
    ncbAutoRegister::AllUnregist();
}

bool TVPLoadInternalPlugin(const ttstr &_name)
{
    /* 1. 拿到 ttstr 的原始缓冲区 */
    const tjs_char *src = _name.c_str();
    size_t len          = _name.length();

    /* 2. 在 src 里找最后一个 '/' 或 '\\'，定位纯文件名起始 */
    const tjs_char *fileBegin = src;
    for (const tjs_char *p = src; *p; ++p)
    {
        if (*p == L'/' || *p == L'\\')
            fileBegin = p + 1;
    }

    /* 3. 在 fileBegin 里找最后一个 '.' */
    const tjs_char *dot = nullptr;
    for (const tjs_char *p = fileBegin; *p; ++p)
    {
        if (*p == L'.')
            dot = p;          // 记录最后一个 '.'
    }

    /* 4. 检查后缀 .tpm（不区分大小写） */
    bool needReplace = false;
    if (dot && dot[1] && dot[2] && dot[3] && !dot[4])   // 长度正好 4：".tpm"
    {
        tjs_char low[5];                 // 存放小写副本
        for (int i = 0; i < 4; ++i)
            low[i] = (tjs_char)towlower(dot[i]);
        low[4] = 0;

        if (TJS_strncmp(low, TJS_W(".tpm"), 4) == 0)
            needReplace = true;
    }

    /* 5. 构造结果字符串 */
    if (needReplace)
    {
        /* 需要替换为 .dll，计算新长度 */
        size_t newLen = len - 3 + 3;     // 去掉 "tpm" 加上 "dll"
        tjs_char *buf = new tjs_char[newLen + 1];

        /* 拷贝前缀（含 .） */
        TJS_strncpy(buf, src, dot - src + 1);
        buf[dot - src + 1] = 0;

        /* 追加 dll */
        TJS_strcat(buf, TJS_W("dll"));

        ttstr fixed(buf);
        delete[] buf;

        return ncbAutoRegister::LoadModule(TVPExtractStorageName(fixed));
    }
    else
    {
        /* 无需处理，直接使用原串 */
        return ncbAutoRegister::LoadModule(TVPExtractStorageName(_name));
    }
}