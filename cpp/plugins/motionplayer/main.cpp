//
// Created by LiDon on 2025/9/13.
// TODO: implement emoteplayer.dll plugin
//
#include <spdlog/spdlog.h>
#include "tjs.h"
#include "ncbind.hpp"

#define NCB_MODULE_NAME TJS_W("emoteplayer.dll")

#define LOGGER spdlog::get("plugin")
class Player {};

NCB_REGISTER_SUBCLASS_DELAY(Player) { NCB_CONSTRUCTOR(()); }

class EmotePlayer {};

NCB_REGISTER_SUBCLASS_DELAY(EmotePlayer) { NCB_CONSTRUCTOR(()); }

class ResourceManager {
public:
    explicit ResourceManager() = default;
    explicit ResourceManager(tTJSVariant v1, tTJSVariant v2) {
        assert(v1.Type() == tvtObject && v2.Type() == tvtInteger);
    }
    static tjs_error setEmotePSBDecryptSeed(tTJSVariant *r, tjs_int n,
                                            tTJSVariant **p,
                                            iTJSDispatch2 *obj) {
        LOGGER->info("setEmotePSBDecryptSeed: {}", static_cast<tjs_int>(*p[0]));
        return TJS_S_OK;
    }

    static tjs_error setEmotePSBDecryptFunc(tTJSVariant *r, tjs_int n,
                                            tTJSVariant **p,
                                            iTJSDispatch2 *obj) {
        // LOGGER->info("setEmotePSBDecryptFunc: {}", a);
        return TJS_S_OK;
    }
};

NCB_REGISTER_SUBCLASS_DELAY(ResourceManager) {
    NCB_CONSTRUCTOR((tTJSVariant, tTJSVariant));
    NCB_METHOD_RAW_CALLBACK(setEmotePSBDecryptSeed,
                            &ResourceManager::setEmotePSBDecryptSeed,
                            TJS_STATICMEMBER);
    NCB_METHOD_RAW_CALLBACK(setEmotePSBDecryptFunc,
                            &ResourceManager::setEmotePSBDecryptFunc,
                            TJS_STATICMEMBER);
}

class Motion {};

NCB_REGISTER_CLASS(Motion) {
    NCB_SUBCLASS(ResourceManager, ResourceManager);
    NCB_SUBCLASS(Player, Player);
    NCB_SUBCLASS(EmotePlayer, EmotePlayer);
}

static void PreRegistCallback() {}

static void PostUnregistCallback() {}

NCB_PRE_REGIST_CALLBACK(PreRegistCallback);
NCB_POST_UNREGIST_CALLBACK(PostUnregistCallback);
