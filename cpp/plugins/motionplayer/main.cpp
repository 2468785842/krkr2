//
// Created by LiDon on 2025/9/13.
// TODO: implement emoteplayer.dll(motionplayer.dll) plugin
//
#include <spdlog/spdlog.h>
#include "tjs.h"
#include "ncbind.hpp"
#include "psbfile/PSBFile.h"

#include "ResourceManager.h"
#include "EmotePlayer.h"
#include "Player.h"
#include "SeparateLayerAdaptor.h"

using namespace emote;

#define NCB_MODULE_NAME TJS_W("motionplayer.dll")
#define LOGGER spdlog::get("plugin")

NCB_TYPECONV_CAST_INTEGER(LayerType);
NCB_TYPECONV_CAST_INTEGER(ShapeType);
NCB_TYPECONV_CAST_INTEGER(CoordinateRecutangular);
NCB_TYPECONV_CAST_INTEGER(TransformOrderType);
NCB_TYPECONV_CAST_INTEGER(PlayFlagForceType);
NCB_TYPECONV_CAST_INTEGER(MaskModeType);
NCB_TYPECONV_CAST_INTEGER(tTVPBBStretchType);
NCB_TYPECONV_CAST_INTEGER(EmotePlayer::TimelinePlayFlagsType);

NCB_REGISTER_SUBCLASS_DELAY(SeparateLayerAdaptor) { NCB_CONSTRUCTOR(()); }

NCB_REGISTER_SUBCLASS_DELAY(Player) { NCB_CONSTRUCTOR(()); }

NCB_REGISTER_SUBCLASS_DELAY(EmotePlayer) {
    Variant("TimelinePlayFlagParallel",
            EmotePlayer::TimelinePlayFlagsType::Parallel);
    Variant("TimelinePlayFlagDifference",
            EmotePlayer::TimelinePlayFlagsType::Difference);
    NCB_CONSTRUCTOR((ResourceManager));
    NCB_PROPERTY(useD3D, getUseD3D, setUseD3D);
    NCB_PROPERTY(motionKey, getMotionKey, setMotionKey);
    NCB_PROPERTY(chara, getChara, setChara);
    NCB_PROPERTY(maskMode, getMaskMode, setMaskMode);
    NCB_PROPERTY(completionType, getCompletionType, setCompletionType);
    NCB_METHOD(play);
    NCB_METHOD(initPhysics);
    NCB_METHOD(draw);
    NCB_METHOD(getMainTimelineLabelList);
}

NCB_REGISTER_SUBCLASS(ResourceManager) {
    NCB_CONSTRUCTOR((iTJSDispatch2 *, tjs_int));
    NCB_METHOD(load);
    NCB_METHOD_RAW_CALLBACK(setEmotePSBDecryptSeed,
                            &ResourceManager::setEmotePSBDecryptSeed,
                            TJS_STATICMEMBER);
    NCB_METHOD_RAW_CALLBACK(setEmotePSBDecryptFunc,
                            &ResourceManager::setEmotePSBDecryptFunc,
                            TJS_STATICMEMBER);
}

class Motion {
public:
    static tjs_error setEnableD3D(tTJSVariant *, tjs_int count, tTJSVariant **p,
                                  iTJSDispatch2 *) {
        if(count == 1 && (*p)->Type() == tvtInteger) {
            _enableD3D = static_cast<bool>((*p)->AsInteger());
            return TJS_S_OK;
        }
        return TJS_E_INVALIDPARAM;
    }

    static tjs_error getEnableD3D(tTJSVariant *r, tjs_int, tTJSVariant **,
                                  iTJSDispatch2 *) {
        *r = tTJSVariant{ _enableD3D };
        return TJS_S_OK;
    }

private:
    inline static bool _enableD3D;
};

NCB_REGISTER_CLASS(Motion) {

    Variant("LayerTypeObj", LayerType::Obj);
    Variant("LayerTypeShape", LayerType::Shape);
    Variant("LayerTypeLayout", LayerType::Layout);
    Variant("LayerTypeMotion", LayerType::Motion);
    Variant("LayerTypeParticle", LayerType::Particle);
    Variant("LayerTypeCamera", LayerType::Camera);

    Variant("ShapeTypePoint", ShapeType::Point);
    Variant("ShapeTypeCircle", ShapeType::Circle);
    Variant("ShapeTypeRect", ShapeType::Rect);
    Variant("ShapeTypeQuad", ShapeType::Quad);

    Variant("PlayFlagForce", PlayFlagForceType::Force);
    Variant("PlayFlagChain", PlayFlagForceType::Chain);
    Variant("PlayFlagAsCan", PlayFlagForceType::AsCan);
    Variant("PlayFlagJoin", PlayFlagForceType::Join);
    Variant("PlayFlagStealth", PlayFlagForceType::Stealth);

    Variant("TransformOrderFlip", TransformOrderType::Flip);
    Variant("TransformOrderSlant", TransformOrderType::Angle);
    Variant("TransformOrderZoom", TransformOrderType::Zoom);
    Variant("TransformOrderAngle", TransformOrderType::Slant);

    Variant("CoordinateRecutangularXY", CoordinateRecutangular::XY);
    Variant("CoordinateRecutangularXZ", CoordinateRecutangular::XZ);

    Variant("MaskModeStencil", MaskModeType::Stencil);
    Variant("MaskModeAlpha", MaskModeType::Alpha);
    NCB_PROPERTY_RAW_CALLBACK(enableD3D, Motion::getEnableD3D,
                              Motion::setEnableD3D, TJS_STATICMEMBER);
    // Point
    // Circle
    // Rect
    // Quad
    // LayerGetter
    // LayerSetter
    // SourceCache
    // ObjSource
    NCB_SUBCLASS(ResourceManager, ResourceManager);
    NCB_SUBCLASS(Player, Player);
    NCB_SUBCLASS(EmotePlayer, EmotePlayer);
    NCB_SUBCLASS(SeparateLayerAdaptor, SeparateLayerAdaptor);
}

static void PreRegistCallback() {}

static void PostUnregistCallback() {}

NCB_PRE_REGIST_CALLBACK(PreRegistCallback);
NCB_POST_UNREGIST_CALLBACK(PostUnregistCallback);
