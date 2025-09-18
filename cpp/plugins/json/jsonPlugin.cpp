#include "tp_stub.h"
#include "ncbind.hpp"
#include <sstream>
#include <locale>

//-------------------------------------------------

#define NCB_MODULE_NAME TJS_W("json.dll")

#include <fstream>
#include <string>


// 静态工具类
struct Scripts {
    // 1) evalJSON
    static tjs_error evalJSON(tTJSVariant *result, tjs_int numparams,
                              tTJSVariant **param, iTJSDispatch2 *objthis) {
        if(numparams < 1)
            return TJS_E_BADPARAMCOUNT;

        ttstr jsonText = *param[0];
        // 这里简单示范：直接返回原字符串给脚本（实际应解析）
        if(result)
            *result = jsonText;
        return TJS_S_OK;
    }

    // 2) evalJSONStorage
    static tjs_error evalJSONStorage(tTJSVariant *result, tjs_int numparams,
                                     tTJSVariant **param,
                                     iTJSDispatch2 *objthis) {
        if(numparams < 1)
            return TJS_E_BADPARAMCOUNT;

        ttstr fileName = TVPGetPlacedPath(*param[0]);
        bool utf8 = numparams > 1 ? (tjs_int)*param[1] != 0 : false;

        // 这里只是示范：读文件并返回文本
        ttstr content;
        try {
            content = TVPLoadText(fileName, TJS_W("z1"));
        } catch(...) {
            return TJS_S_FALSE;
        }
        if(!content.IsEmpty()) {
            if(result)
                *result = content; // 实际应解析 JSON
        } else {
            if(result)
                result->Clear();
        }
        return TJS_S_OK;
    }

    // 3) saveJSON
    static tjs_error saveJSON(tTJSVariant *result, tjs_int numparams,
                              tTJSVariant **param, iTJSDispatch2 *objthis) {
        if(numparams < 2)
            return TJS_E_BADPARAMCOUNT;

        ttstr fileName = TVPGetPlacedPath(*param[0]);
        tTJSVariant obj = *param[1];
        bool utf8 = numparams > 2 ? (tjs_int)*param[2] != 0 : false;
        int newline = numparams > 3 ? (tjs_int)*param[3] : 0;

        // 模拟序列化：直接把对象转成字符串（实际应使用 JSON 库）
        ttstr jsonStr = obj.AsStringNoAddRef();
        try {
            TVPSaveText(fileName, jsonStr, TJS_W("z1"));
        } catch(...) {
            if(result)
                *result = false;
        }
        if(result)
            *result = true;
        return TJS_S_OK;
    }

    // 4) toJSONString
    static tjs_error toJSONString(tTJSVariant *result, tjs_int numparams,
                                  tTJSVariant **param, iTJSDispatch2 *objthis) {
        if(numparams < 1)
            return TJS_E_BADPARAMCOUNT;

        tTJSVariant obj = *param[0];
        int newline = numparams > 1 ? (tjs_int)*param[1] : 0;

        // 简单示范：把对象文本化
        ttstr str = obj.AsStringNoAddRef();
        if(result)
            *result = str;
        return TJS_S_OK;
    }
};

// 注册到 TJS 全局命名空间
NCB_ATTACH_CLASS(Scripts, Scripts) {
    RawCallback("evalJSON", &Scripts::evalJSON, TJS_STATICMEMBER);
    RawCallback("evalJSONStorage", &Scripts::evalJSONStorage, TJS_STATICMEMBER);
    RawCallback("saveJSON", &Scripts::saveJSON, TJS_STATICMEMBER);
    RawCallback("toJSONString", &Scripts::toJSONString, TJS_STATICMEMBER);
}