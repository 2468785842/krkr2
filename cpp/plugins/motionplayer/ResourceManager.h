//
// Created by LiDon on 2025/9/15.
//
#pragma once

#include "tjs.h"
#include "WindowImpl.h"
#include "psbfile/PSBFile.h"

namespace emote {

    class ResourceManager {
    public:
        explicit ResourceManager() = default;

        explicit ResourceManager(iTJSDispatch2 *window, tjs_int cacheSize);

        tTJSVariant load(ttstr path);

        const PSB::PSBFile *getPSBFile() const {
            return &_psbFile;
        }

        static tjs_error setEmotePSBDecryptSeed(tTJSVariant *r, tjs_int count,
                                                tTJSVariant **p,
                                                iTJSDispatch2 *obj);

        static tjs_error setEmotePSBDecryptFunc(tTJSVariant *r, tjs_int n,
                                                tTJSVariant **p,
                                                iTJSDispatch2 *obj);

    private:
        PSB::PSBFile _psbFile;
        iTJSDispatch2 *_window; // tTJSNC_Window
        tjs_int _cacheSize;
        inline static int _decryptSeed;
    };
} // namespace emote