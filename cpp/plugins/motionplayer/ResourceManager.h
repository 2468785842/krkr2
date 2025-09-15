//
// Created by LiDon on 2025/9/15.
//
#pragma once
#include "tjs.h"

namespace motion {

    class ResourceManager {
    public:
        explicit ResourceManager() = default;

        explicit ResourceManager(iTJSDispatch2 *kag, tjs_int cacheSize);

        tTJSVariant load(ttstr path) const;

        static tjs_error setEmotePSBDecryptSeed(tTJSVariant *r, tjs_int count,
                                                tTJSVariant **p,
                                                iTJSDispatch2 *obj);

        static tjs_error setEmotePSBDecryptFunc(tTJSVariant *r, tjs_int n,
                                                tTJSVariant **p,
                                                iTJSDispatch2 *obj);

    private:
        inline static int _decryptSeed;
    };
} // namespace motion