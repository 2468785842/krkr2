#pragma once

#include "tjsObject.h"

namespace TJS {
    // C++17 boundary: Ciallang/VM types stay inside the C++20 adapter.
    class CiallangRuntime {
    public:
        virtual ~CiallangRuntime() = default;
        virtual tTJSCustomObject *Global() = 0;
        virtual void Execute(const ttstr &source, tTJSVariant *result,
                             iTJSDispatch2 *context, const ttstr &name,
                             tjs_int lineOffset, bool expression) = 0;
        virtual void SetPPValue(const tjs_char *name, tjs_int32 value) = 0;
        virtual void Collect() = 0;
        virtual void Shutdown() = 0;
    };

    class tTJS;
    CiallangRuntime *AttachCiallangRuntime(tTJS *engine,
                                           iTJSDispatch2 *nativeGlobal);
    CiallangRuntime *FindCiallangRuntime(tTJS *engine);
    void ReleaseCiallangRuntime(tTJS *engine);
} // namespace TJS
