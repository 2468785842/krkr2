#include "tp_stub.h"
#include "ncbind/ncbind.hpp"
#include <sstream>
#include <locale>

//-------------------------------------------------

#define NCB_MODULE_NAME TJS_W("json.dll")

#include <fstream>
#include <string>

#if defined(_WIN32)
#   include <windows.h>
#else
#   include <codecvt>
#endif

// 把宽字符串转成当前 locale 的窄字符串（≈ CP_ACP）
static inline std::string wideToLocale(const std::wstring& w)
{
    if (w.empty()) return {};

#if defined(_WIN32)
    int len = WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string out(len - 1, '\0');
    WideCharToMultiByte(CP_ACP, 0, w.c_str(), -1, &out[0], len, nullptr, nullptr);
    return out;
#else
    // Linux/macOS：使用 wstring_convert + codecvt
    std::wstring_convert<std::codecvt_byname<wchar_t, char, std::mbstate_t>> conv("");
    try {
        return conv.to_bytes(w);
    } catch (...) {
        return {};   // 转换失败
    }
#endif
}

// 把 ttstr → UTF-8 字节序列
static inline std::string toUTF8(const ttstr& s)
{
    const wchar_t* w = reinterpret_cast<const wchar_t*>(s.c_str());
    if (!w || !*w) return {};

#if defined(_WIN32)
    // Windows: WideCharToMultiByte
    int len = WideCharToMultiByte(CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string out(len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, w, -1, &out[0], len, nullptr, nullptr);
    return out;
#else
    // Linux / macOS: codecvt (deprecated but still usable)
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
    return conv.to_bytes(w);
#endif
}

// 保存文本文件（UTF-8 或当前代码页）
tjs_error TVPSaveText(const ttstr& filename,
                      const ttstr& text,
                      const tjs_char* encoding,   // "utf-8" 或 nullptr
                      const tjs_char* newline)    // "\n" 或 "\r\n"
{
    std::ofstream ofs;
    std::ios_base::openmode mode = std::ios::out;
    if (encoding && wcscmp(reinterpret_cast<const wchar_t*>(encoding),L"utf-8") == 0)
        mode |= std::ios::binary;  // 防止 CRT 转换

    ofs.open(reinterpret_cast<const wchar_t*>(filename.c_str()), mode);
    if (!ofs)
        return TJS_E_FAIL;

    std::string out;
    if (encoding && wcscmp(reinterpret_cast<const wchar_t*>(encoding), L"utf-8") == 0)
        out = toUTF8(text);
    else
    {   // 当前代码页
        out = wideToLocale(std::wstring(reinterpret_cast<const wchar_t*>(text.c_str())));
    }

    // 替换换行符
    std::string nl = newline ? toUTF8(newline) : "\r\n";
    for (size_t p = 0; (p = out.find('\n', p)) != std::string::npos; )
    {
        out.replace(p, 1, nl);
        p += nl.size();
    }

    ofs << out;
    return ofs ? TJS_S_OK : TJS_E_FAIL;
}

// UTF-8 → wide
static inline std::wstring fromUTF8(const std::string& s)
{
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(len, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], len);
    return out;
}

// 读取文本文件 → ttstr
tjs_error TVPLoadText(const ttstr& filename,
                      ttstr& text,
                      const tjs_char* encoding)   // "utf-8" 或 nullptr
{
    std::wstring wfile = reinterpret_cast<const wchar_t*>(filename.c_str());
    std::ifstream ifs(wfile, std::ios::binary);
    if (!ifs)
        return TJS_E_FAIL;

    // 读全部字节
    std::string raw((std::istreambuf_iterator<char>(ifs)),
                    std::istreambuf_iterator<char>());

    // 转成宽字符串
    if (encoding && wcscmp(reinterpret_cast<const wchar_t*>(encoding), L"utf-8") == 0)
        text = ttstr(fromUTF8(raw).c_str());
    else
    {   // 当前代码页
        int len = MultiByteToWideChar(CP_ACP, 0, raw.c_str(), -1, nullptr, 0);
        std::wstring w(len, 0);
        MultiByteToWideChar(CP_ACP, 0, raw.c_str(), -1, &w[0], len);
        text = ttstr(w.c_str());
    }

    return TJS_S_OK;
}
// 静态工具类
struct Scripts
{
    // 1) evalJSON
    static tjs_error  evalJSON(
        tTJSVariant *result,
        tjs_int numparams,
        tTJSVariant **param,
        iTJSDispatch2 *objthis)
    {
        if (numparams < 1) return TJS_E_BADPARAMCOUNT;

        ttstr jsonText = *param[0];
        // 这里简单示范：直接返回原字符串给脚本（实际应解析）
        if (result) *result = jsonText;
        return TJS_S_OK;
    }

    // 2) evalJSONStorage
    static tjs_error  evalJSONStorage(
        tTJSVariant *result,
        tjs_int numparams,
        tTJSVariant **param,
        iTJSDispatch2 *objthis)
    {
        if (numparams < 1) return TJS_E_BADPARAMCOUNT;

        ttstr fileName = TVPGetPlacedPath(*param[0]);
        bool utf8 = numparams > 1 ? (tjs_int)*param[1] != 0 : false;

        // 这里只是示范：读文件并返回文本
        ttstr content;
        if (TVPLoadText(fileName, content, utf8 ? TJS_W("utf-8") : 0) == TJS_S_OK)
        {
            if (result) *result = content; // 实际应解析 JSON
        }
        else
        {
            if (result) result->Clear();
        }
        return TJS_S_OK;
    }

    // 3) saveJSON
    static tjs_error  saveJSON(
        tTJSVariant *result,
        tjs_int numparams,
        tTJSVariant **param,
        iTJSDispatch2 *objthis)
    {
        if (numparams < 2) return TJS_E_BADPARAMCOUNT;

        ttstr fileName = TVPGetPlacedPath(*param[0]);
        tTJSVariant obj = *param[1];
        bool utf8   = numparams > 2 ? (tjs_int)*param[2] != 0 : false;
        int newline = numparams > 3 ? (tjs_int)*param[3] : 0;

        // 模拟序列化：直接把对象转成字符串（实际应使用 JSON 库）
        ttstr jsonStr = obj.AsStringNoAddRef();
        if (TVPSaveText(fileName, jsonStr,
                        utf8 ? TJS_W("utf-8") : 0,
                        newline ? TJS_W("\n") : TJS_W("\r\n")) == TJS_S_OK)
        {
            if (result) *result = true;
        }
        else
        {
            if (result) *result = false;
        }
        return TJS_S_OK;
    }

    // 4) toJSONString
    static tjs_error  toJSONString(
        tTJSVariant *result,
        tjs_int numparams,
        tTJSVariant **param,
        iTJSDispatch2 *objthis)
    {
        if (numparams < 1) return TJS_E_BADPARAMCOUNT;

        tTJSVariant obj = *param[0];
        int newline = numparams > 1 ? (tjs_int)*param[1] : 0;

        // 简单示范：把对象文本化
        ttstr str = obj.AsStringNoAddRef();
        if (result) *result = str;
        return TJS_S_OK;
    }
};

// 注册到 TJS 全局命名空间
NCB_ATTACH_CLASS(Scripts, Scripts)
{
    RawCallback("evalJSON",        &Scripts::evalJSON,        TJS_STATICMEMBER);
    RawCallback("evalJSONStorage", &Scripts::evalJSONStorage, TJS_STATICMEMBER);
    RawCallback("saveJSON",        &Scripts::saveJSON,        TJS_STATICMEMBER);
    RawCallback("toJSONString",    &Scripts::toJSONString,    TJS_STATICMEMBER);
}