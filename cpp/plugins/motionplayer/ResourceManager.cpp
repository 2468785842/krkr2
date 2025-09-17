//
// Created by LiDon on 2025/9/15.
//

#include "ResourceManager.h"

#include "tjsObject.h"
#include "psbfile/PSBFile.h"

#define LOGGER spdlog::get("plugin")

motion::ResourceManager::ResourceManager(iTJSDispatch2 *kag,
                                         tjs_int cacheSize) {
    LOGGER->info("kag: {}, cacheSize: {}", static_cast<void *>(kag), cacheSize);
}

tjs_error motion::ResourceManager::setEmotePSBDecryptSeed(tTJSVariant *,
                                                          tjs_int count,
                                                          tTJSVariant **p,
                                                          iTJSDispatch2 *) {
    if(count != 1 && (*p)->Type() == tvtInteger) {
        return TJS_E_BADPARAMCOUNT;
    }
    _decryptSeed = static_cast<tjs_int>(*p[0]);
    LOGGER->info("setEmotePSBDecryptSeed: {}", _decryptSeed);
    return TJS_S_OK;
}

tjs_error motion::ResourceManager::setEmotePSBDecryptFunc(tTJSVariant *r,
                                                          tjs_int n,
                                                          tTJSVariant **p,
                                                          iTJSDispatch2 *obj) {
    LOGGER->critical("setEmotePSBDecryptFunc no implement!");
    return TJS_S_OK;
}

tTJSVariant motion::ResourceManager::load(ttstr path) const {
    PSB::PSBFile f;
    f.setSeed(_decryptSeed);
    if(!f.loadPSBFile(path)) {
        LOGGER->error("emote load file: {} failed", path.AsStdString());
    }

    iTJSDispatch2 *dic = TJSCreateCustomObject();
    auto objs = f.getObjects();
    if(objs != nullptr) {
        for(const auto &[k, v] : *objs) {
            tTJSVariant tmp = v->toTJSVal();
            dic->PropSet(TJS_MEMBERENSURE, ttstr{ k }.c_str(), nullptr, &tmp,
                         dic);
        }
    }
    tTJSVariant result{ dic, dic };
    dic->Release();
    return result;
}