//
// Created by LiDon on 2025/9/13.
// Reverse-engineered from libkrkr2.so emoteplayer.dll + motionplayer.dll
//
#include "tjs.h"
#include "ncbind.hpp"
#include "psbfile/PSBFile.h"

#include "ResourceManager.h"
#include "EmotePlayer.h"
#include "Player.h"
#include "SeparateLayerAdaptor.h"
#include "D3DEmoteModule.h"
#include "SourceCache.h"
#include "D3DAdaptor.h"

using namespace motion;

#define NCB_MODULE_NAME TJS_W("motionplayer.dll")
#include "log/TVPLog.h"

// ============================================================
// Subclass registrations (used as Motion.XXX)
// ============================================================

NCB_REGISTER_SUBCLASS_DELAY(SourceCache) {
    NCB_CONSTRUCTOR((tTJSVariant, tjs_int));
    NCB_METHOD(loadSource);
    NCB_METHOD(clearCache);
    NCB_PROPERTY_RO(bufLayer, getBufLayer);
}
NCB_REGISTER_SUBCLASS_DELAY(ObjSource) { NCB_CONSTRUCTOR(()); }

// Aligned to libkrkr2.so Motion.Point/Circle/Rect/Quad/LayerGetter
// (0x690FBC~0x69B350)
NCB_REGISTER_SUBCLASS_DELAY(Point) {
    NCB_CONSTRUCTOR(());
    NCB_PROPERTY_RO(type, getType);
    NCB_METHOD(contains);
    NCB_PROPERTY_RO(x, getX);
    NCB_PROPERTY_RO(y, getY);
}
NCB_REGISTER_SUBCLASS_DELAY(Circle) {
    NCB_CONSTRUCTOR(());
    NCB_PROPERTY_RO(type, getType);
    NCB_METHOD(contains);
    NCB_PROPERTY_RO(x, getX);
    NCB_PROPERTY_RO(y, getY);
    NCB_PROPERTY_RO(r, getR);
}
NCB_REGISTER_SUBCLASS_DELAY(Rect) {
    NCB_CONSTRUCTOR(());
    NCB_PROPERTY_RO(type, getType);
    NCB_METHOD(contains);
    NCB_PROPERTY_RO(l, getL);
    NCB_PROPERTY_RO(t, getT);
    NCB_PROPERTY_RO(w, getW);
    NCB_PROPERTY_RO(h, getH);
}
NCB_REGISTER_SUBCLASS_DELAY(Quad) {
    NCB_CONSTRUCTOR(());
    NCB_PROPERTY_RO(type, getType);
    NCB_METHOD(contains);
    NCB_PROPERTY_RO(p, getP);
}
NCB_REGISTER_SUBCLASS_DELAY(LayerGetter) {
    NCB_CONSTRUCTOR(());
    NCB_PROPERTY_RO(type, getType);
    NCB_PROPERTY_RO(label, getLabel);
    NCB_PROPERTY_RO(visible, getVisible);
    NCB_PROPERTY_RO(branchVisible, getBranchVisible);
    NCB_PROPERTY_RO(layerVisible, getLayerVisible);
    NCB_PROPERTY_RO(x, getX);
    NCB_PROPERTY_RO(y, getY);
    NCB_PROPERTY_RO(left, getLeft);
    NCB_PROPERTY_RO(top, getTop);
    NCB_PROPERTY_RO(flipX, getFlipX);
    NCB_PROPERTY_RO(flipY, getFlipY);
    NCB_PROPERTY_RO(zoomX, getZoomX);
    NCB_PROPERTY_RO(zoomY, getZoomY);
    NCB_PROPERTY_RO(angleDeg, getAngleDeg);
    NCB_PROPERTY_RO(angleRad, getAngleRad);
    NCB_PROPERTY_RO(slantX, getSlantX);
    NCB_PROPERTY_RO(slantY, getSlantY);
    NCB_PROPERTY_RO(originX, getOriginX);
    NCB_PROPERTY_RO(originY, getOriginY);
    NCB_PROPERTY_RO(opacity, getOpacity);
    NCB_PROPERTY_RO(mtx, getMtx);
    NCB_PROPERTY_RO(vtx, getVtx);
    NCB_PROPERTY_RO(color, getColor);
    NCB_PROPERTY_RO(bezierPatch, getBezierPatch);
    NCB_PROPERTY_RO(shape, getShape);
    NCB_PROPERTY_RO(motion, getMotion);
    NCB_PROPERTY_RO(particle, getParticle);
}
// Aligned to libkrkr2.so SeparateLayerAdaptor_ncb_registerMembers (0x6ABFAC)
NCB_REGISTER_SUBCLASS_DELAY(SeparateLayerAdaptor) {
    Factory(&SeparateLayerAdaptor::factory);
    NCB_PROPERTY(absolute, getAbsolute, setAbsolute);
    NCB_PROPERTY(targetLayer, getTargetLayer, setTargetLayer);
    NCB_METHOD(clear);
    RawCallback(TJS_W("assign"), &SeparateLayerAdaptor::assignCompat, 0);
    RawCallback(TJS_W("layerTreeOwnerInterface"),
                &SeparateLayerAdaptor::getLayerTreeOwnerInterfaceCompat, (int)0,
                TJS_HIDDENMEMBER);
}
NCB_REGISTER_SUBCLASS_DELAY(D3DAdaptor) {
    Factory(&D3DAdaptor::factory);
    NCB_METHOD(setPos);
    NCB_METHOD(setSize);
    NCB_METHOD(setClearColor);
    NCB_METHOD(setResizable);
    NCB_METHOD(removeAllTextures);
    NCB_METHOD(removeAllBg);
    NCB_METHOD(removeAllCaption);
    NCB_METHOD(registerBg);
    NCB_METHOD(registerCaption);
    NCB_METHOD(unloadUnusedTextures);
    RawCallback(TJS_W("captureCanvas"), &D3DAdaptor::captureCanvasStatic, 0);
    NCB_PROPERTY(visible, getVisible, setVisible);
    NCB_PROPERTY(alphaOpAdd, getAlphaOpAdd, setAlphaOpAdd);
    NCB_PROPERTY(canvasCaptureEnabled, getCanvasCaptureEnabled,
                 setCanvasCaptureEnabled);
    NCB_PROPERTY(clearEnabled, getClearEnabled, setClearEnabled);
}

NCB_REGISTER_CLASS(Player) {
    NCB_CONSTRUCTOR((ResourceManager));

    // Properties
    // Root node position — aligned to libkrkr2.so NCB registration (0x6D69C8)
    NCB_PROPERTY(x, getX, setX);
    NCB_PROPERTY(y, getY, setY);
    NCB_PROPERTY(left, getLeft, setLeft);
    NCB_PROPERTY(top, getTop, setTop);

    NCB_PROPERTY(completionType, getCompletionType, setCompletionType);
    NCB_PROPERTY(metadata, getMetadata, setMetadata);
    NCB_PROPERTY(chara, getChara, setChara);
    // Aligned to libkrkr2.so 0x681CAC: raw callback to access objthis
    // for onFindMotion TJS callback during motion loading
    NCB_PROPERTY_RAW_CALLBACK(motion, Player::getMotionCompat,
                              Player::setMotionCompat, 0);
    NCB_PROPERTY(motionKey, getMotionKey, setMotionKey);
    NCB_PROPERTY(outline, getOutline, setOutline);
    NCB_PROPERTY(priorDraw, getPriorDraw, setPriorDraw);
    NCB_PROPERTY(frameLastTime, getFrameLastTime, setFrameLastTime);
    NCB_PROPERTY(frameLoopTime, getFrameLoopTime, setFrameLoopTime);
    NCB_PROPERTY(loopTime, getLoopTime, setLoopTime);
    NCB_PROPERTY(processedMeshVerticesNum, getProcessedMeshVerticesNum,
                 setProcessedMeshVerticesNum);
    NCB_PROPERTY_RO(playing, getPlaying);
    NCB_PROPERTY(queuing, getQueuing, setQueuing);
    NCB_PROPERTY(directEdit, getDirectEdit, setDirectEdit);
    NCB_PROPERTY(selectorEnabled, getSelectorEnabled, setSelectorEnabled);
    NCB_PROPERTY(variableKeys, getVariableKeys, setVariableKeys);
    NCB_PROPERTY_RO(allplaying, getAllplaying);
    NCB_PROPERTY(syncWaiting, getSyncWaiting, setSyncWaiting);
    NCB_PROPERTY(syncActive, getSyncActive, setSyncActive);
    NCB_PROPERTY(hasCamera, getHasCamera, setHasCamera);
    NCB_PROPERTY(cameraActive, getCameraActive, setCameraActive);
    NCB_PROPERTY(stereovisionActive, getStereovisionActive,
                 setStereovisionActive);
    NCB_PROPERTY(tickCount, getTickCount, setTickCount);
    NCB_PROPERTY(speed, getSpeed, setSpeed);
    NCB_PROPERTY(frameTickCount, getFrameTickCount, setFrameTickCount);
    NCB_PROPERTY(maskMode, getMaskMode, setMaskMode);
    NCB_PROPERTY(colorWeight, getColorWeight, setColorWeight);
    NCB_PROPERTY(independentLayerInherit, getIndependentLayerInherit,
                 setIndependentLayerInherit);
    NCB_PROPERTY(zFactor, getZFactor, setZFactor);
    NCB_PROPERTY(cameraTarget, getCameraTarget, setCameraTarget);
    NCB_PROPERTY(cameraPosition, getCameraPosition, setCameraPosition);
    NCB_PROPERTY(cameraFOV, getCameraFOV, setCameraFOV);
    NCB_PROPERTY(cameraAlive, getCameraAlive, setCameraAlive);
    NCB_PROPERTY(canvasCaptureEnabled, getCanvasCaptureEnabled,
                 setCanvasCaptureEnabled);
    NCB_PROPERTY(clearEnabled, getClearEnabled, setClearEnabled);
    NCB_PROPERTY(hitThreshold, getHitThreshold, setHitThreshold);
    NCB_PROPERTY(preview, getPreview, setPreview);
    NCB_PROPERTY(outsideFactor, getOutsideFactor, setOutsideFactor);
    NCB_PROPERTY(resourceManager, getResourceManager, setResourceManager);
    NCB_PROPERTY(stealthChara, getStealthChara, setStealthChara);
    NCB_PROPERTY(stealthMotion, getStealthMotion, setStealthMotion);
    NCB_PROPERTY(tags, getTags, setTags);
    NCB_PROPERTY(project, getProject, setProject);
    NCB_PROPERTY(useD3D, getUseD3D, setUseD3D);
    NCB_PROPERTY(meshline, getMeshline, setMeshline);
    NCB_PROPERTY_RO(busy, getBusy);

    // Core methods
    NCB_METHOD(random);
    NCB_METHOD(initPhysics);
    NCB_METHOD(serialize);
    NCB_METHOD(unserialize);
    NCB_METHOD(setRotate);
    NCB_METHOD(setMirror);
    NCB_METHOD(setHairScale);
    NCB_METHOD(setPartsScale);
    NCB_METHOD(setBustScale);
    NCB_METHOD_RAW_CALLBACK(setDrawAffineTranslateMatrix,
                            &Player::setDrawAffineTranslateMatrixCompat, 0);
    NCB_METHOD(getCameraOffset);
    NCB_METHOD(setCameraOffset);
    NCB_METHOD(modifyRoot);
    NCB_METHOD(debugPrint);

    // Resource management
    NCB_METHOD(unload);
    NCB_METHOD(unloadAll);
    NCB_METHOD(isExistMotion);
    NCB_METHOD(findMotion);
    NCB_METHOD(requireLayerId);
    NCB_METHOD(releaseLayerId);

    // Drawing/rendering
    NCB_METHOD(setClearColor);
    NCB_METHOD(setResizable);
    NCB_METHOD(removeAllTextures);
    NCB_METHOD(removeAllBg);
    NCB_METHOD(removeAllCaption);
    NCB_METHOD(registerBg);
    NCB_METHOD(registerCaption);
    NCB_METHOD(unloadUnusedTextures);
    NCB_METHOD(alphaOpAdd);
    NCB_METHOD_RAW_CALLBACK(captureCanvas, &Player::captureCanvasCompat, 0);
    NCB_METHOD(findSource);
    NCB_METHOD(loadSource);
    NCB_METHOD(clearCache);
    NCB_METHOD(setSize);
    NCB_METHOD(copyRect);
    NCB_METHOD(adjustGamma);
    NCB_METHOD_DETAIL(draw, Class, void, Class::draw, (tTJSVariant));
    NCB_METHOD(frameProgress);

    // Viewport/display
    NCB_METHOD(setFlip);
    NCB_METHOD(setOpacity);
    NCB_METHOD(setVisible);
    NCB_METHOD(setSlant);
    NCB_METHOD(setZoom);
    NCB_METHOD(getLayerNames);
    NCB_METHOD(releaseSyncWait);
    NCB_METHOD(calcViewParam);
    NCB_METHOD(getLayerMotion);
    NCB_METHOD(getLayerGetter);
    NCB_METHOD(getLayerGetterList);
    NCB_METHOD(skipToSync);
    NCB_METHOD(setStereovisionCameraPosition);

    // Timeline/variable queries
    NCB_METHOD_RAW_CALLBACK(setVariable, &Player::setVariableCompatMethod, 0);
    NCB_METHOD(getVariable);
    NCB_METHOD(countVariables);
    NCB_METHOD(getVariableLabelAt);
    NCB_METHOD(countVariableFrameAt);
    NCB_METHOD(getVariableFrameLabelAt);
    NCB_METHOD(getVariableFrameValueAt);
    NCB_METHOD(getTimelinePlaying);
    NCB_METHOD(getVariableRange);
    NCB_METHOD(getVariableFrameList);
    NCB_METHOD(countMainTimelines);
    NCB_METHOD(getMainTimelineLabelAt);
    NCB_METHOD(getMainTimelineLabelList);
    NCB_METHOD(countDiffTimelines);
    NCB_METHOD(getDiffTimelineLabelAt);
    NCB_METHOD(getDiffTimelineLabelList);
    NCB_METHOD(getLoopTimeline);
    NCB_METHOD(countPlayingTimelines);
    NCB_METHOD(getPlayingTimelineLabelAt);
    NCB_METHOD(getPlayingTimelineFlagsAt);
    NCB_METHOD(getTimelineTotalFrameCount);
    NCB_METHOD(playTimeline);
    NCB_METHOD(stopTimeline);
    NCB_METHOD(setTimelineBlendRatio);
    NCB_METHOD(getTimelineBlendRatio);
    NCB_METHOD(fadeInTimeline);
    NCB_METHOD(fadeOutTimeline);
    NCB_METHOD(getPlayingTimelineInfoList);

    // Selector
    NCB_METHOD(isSelectorTarget);
    NCB_METHOD(deactivateSelectorTarget);

    // Misc
    NCB_METHOD(getCommandList);
    NCB_METHOD(getD3DAvailable);
    NCB_METHOD(doAlphaMaskOperation);
    NCB_METHOD(onFindMotion);
    NCB_METHOD_RAW_CALLBACK(play, &Player::playCompat, 0);
    NCB_METHOD_RAW_CALLBACK(progress, &Player::progressCompatMethod, 0);
    NCB_METHOD_RAW_CALLBACK(isPlaying, &Player::isPlayingCompat, 0);
    NCB_METHOD_RAW_CALLBACK(stop, &Player::stopCompat, 0);
    NCB_METHOD(motionList);
    NCB_METHOD(emoteEdit);
}

NCB_REGISTER_SUBCLASS_DELAY(EmotePlayer) {
    NCB_CONSTRUCTOR((ResourceManager));

    // Properties
    NCB_PROPERTY_RO(module, getModule);
    NCB_PROPERTY(completionType, getCompletionType, setCompletionType);
    NCB_PROPERTY(chara, getChara, setChara);
    NCB_PROPERTY(motion, getMotion, setMotion);
    NCB_PROPERTY(motionKey, getMotionKey, setMotionKey);
    NCB_PROPERTY(maskMode, getMaskMode, setMaskMode);
    NCB_PROPERTY(outline, getOutline, setOutline);
    NCB_PROPERTY(priorDraw, getPriorDraw, setPriorDraw);
    NCB_PROPERTY(frameLastTime, getFrameLastTime, setFrameLastTime);
    NCB_PROPERTY(frameLoopTime, getFrameLoopTime, setFrameLoopTime);
    NCB_PROPERTY(loopTime, getLoopTime, setLoopTime);
    NCB_PROPERTY(processedMeshVerticesNum, getProcessedMeshVerticesNum,
                 setProcessedMeshVerticesNum);
    NCB_PROPERTY(visible, getVisible, setVisible);
    NCB_PROPERTY(smoothing, getSmoothing, setSmoothing);
    NCB_PROPERTY(meshDivisionRatio, getMeshDivisionRatio, setMeshDivisionRatio);
    NCB_PROPERTY(queing, getQueuing, setQueuing); // original typo preserved
    NCB_PROPERTY(hairScale, getHairScale, setHairScale);
    NCB_PROPERTY(partsScale, getPartsScale, setPartsScale);
    NCB_PROPERTY(bustScale, getBustScale, setBustScale);
    NCB_PROPERTY(bodyScale, getBodyScale, setBodyScale);
    NCB_PROPERTY(useD3D, getUseD3D, setUseD3D);
    NCB_PROPERTY(progress, getProgress, setProgress);
    NCB_PROPERTY(modified, getModified, setModified);
    NCB_PROPERTY(drawvisible, getDrawVisible, setDrawVisible);
    NCB_PROPERTY(drawOpacity, getDrawOpacity, setDrawOpacity);
    NCB_PROPERTY(opengl, getOpengl, setOpengl);
    NCB_PROPERTY_RO(animating, getAnimating);
    NCB_PROPERTY_RO(playCallback, getPlayCallback);

    // Methods
    NCB_METHOD(create);
    NCB_METHOD(load);
    NCB_METHOD(clone);
    NCB_METHOD(serialize);
    NCB_METHOD(unserialize);
    NCB_METHOD(show);
    NCB_METHOD(hide);
    NCB_METHOD(assignState);
    NCB_METHOD(initPhysics);
    NCB_METHOD_RAW_CALLBACK(setRot, &EmotePlayer::setRotCompat, 0);
    NCB_METHOD_RAW_CALLBACK(setRotate, &EmotePlayer::setRotCompat, 0);
    NCB_METHOD(getRot);
    NCB_METHOD_RAW_CALLBACK(setCoord, &EmotePlayer::setCoordCompat, 0);
    NCB_METHOD_RAW_CALLBACK(setScale, &EmotePlayer::setScaleCompat, 0);
    NCB_METHOD(getScale);
    NCB_METHOD(setMirror);
    NCB_METHOD_RAW_CALLBACK(setColor, &EmotePlayer::setColorCompat, 0);
    NCB_METHOD(getColor);
    NCB_METHOD(countVariables);
    NCB_METHOD(getVariableLabelAt);
    NCB_METHOD(countVariableFrameAt);
    NCB_METHOD(getVariableFrameLabelAt);
    NCB_METHOD(getVariableFrameValueAt);
    NCB_METHOD_RAW_CALLBACK(setVariable, &EmotePlayer::setVariableCompat, 0);
    NCB_METHOD(getVariable);
    NCB_METHOD(getVariableFrameList);
    NCB_METHOD_RAW_CALLBACK(startWind, &EmotePlayer::startWindCompat, 0);
    NCB_METHOD_RAW_CALLBACK(stopWind, &EmotePlayer::stopWindCompat, 0);
    NCB_METHOD(countMainTimelines);
    NCB_METHOD(getMainTimelineLabelAt);
    NCB_METHOD(getMainTimelineLabelList);
    NCB_METHOD(countDiffTimelines);
    NCB_METHOD(getDiffTimelineLabelAt);
    NCB_METHOD(getDiffTimelineLabelList);
    NCB_METHOD(countPlayingTimelines);
    NCB_METHOD(getPlayingTimelineLabelAt);
    NCB_METHOD(getPlayingTimelineFlagsAt);
    NCB_METHOD(isLoopTimeline);
    NCB_METHOD(getLoopTimeline);
    NCB_METHOD(getTimelineTotalFrameCount);
    NCB_METHOD(play);
    NCB_METHOD(playTimeline);
    NCB_METHOD(isTimelinePlaying);
    NCB_METHOD(getTimelinePlaying);
    NCB_METHOD(stopTimeline);
    NCB_METHOD(setTimeline);
    NCB_METHOD(setTimelineBlendRatio);
    NCB_METHOD(getTimelineBlendRatio);
    NCB_METHOD(fadeInTimeline);
    NCB_METHOD(fadeOutTimeline);
    NCB_METHOD(getPlayingTimelineInfoList);
    NCB_METHOD(skip);
    NCB_METHOD(skipToSync);
    NCB_METHOD(addPlayCallback);
    NCB_METHOD(pass);
    NCB_METHOD(progress);
    NCB_METHOD_DETAIL(draw, Class, void, Class::draw, (tTJSVariant));
    NCB_METHOD_RAW_CALLBACK(setDrawAffineTranslateMatrix,
                            &EmotePlayer::setDrawAffineTranslateMatrixCompat,
                            0);
    NCB_METHOD_RAW_CALLBACK(setOuterForce, &EmotePlayer::setOuterForceCompat,
                            0);
    NCB_METHOD(getOuterForce);
    NCB_METHOD_RAW_CALLBACK(contains, &EmotePlayer::containsCompat, 0);
}

// ============================================================
// ResourceManager (existing, unchanged)
// ============================================================

NCB_REGISTER_SUBCLASS(ResourceManager) {
    NCB_CONSTRUCTOR((iTJSDispatch2 *, tjs_int));
    NCB_METHOD(load);
    NCB_METHOD(loadSource);
    NCB_METHOD(unload);
    NCB_METHOD(clearCache);
    NCB_METHOD(findSource);
    NCB_METHOD(requireLayerId);
    NCB_METHOD(releaseLayerId);
    NCB_METHOD_RAW_CALLBACK(setEmotePSBDecryptSeed,
                            &ResourceManager::setEmotePSBDecryptSeed,
                            TJS_STATICMEMBER);
    NCB_METHOD_RAW_CALLBACK(setEmotePSBDecryptFunc,
                            &ResourceManager::setEmotePSBDecryptFunc,
                            TJS_STATICMEMBER);
}

// ============================================================
// Motion top-level class with constants and subclasses
// ============================================================

class Motion {};

NCB_REGISTER_CLASS(Motion) {
    // Subclasses (Player registered as top-level class, aliased in
    // PostRegistCallback)
    NCB_SUBCLASS(ResourceManager, ResourceManager);
    NCB_SUBCLASS(EmotePlayer, EmotePlayer);
    NCB_SUBCLASS(SeparateLayerAdaptor, SeparateLayerAdaptor);
    NCB_SUBCLASS(D3DAdaptor, D3DAdaptor);
    NCB_SUBCLASS(SourceCache, SourceCache);
    NCB_SUBCLASS(ObjSource, ObjSource);
    // Aligned to libkrkr2.so Motion_namespace_ncb_register (0x6D9B08)
    NCB_SUBCLASS(Point, Point);
    NCB_SUBCLASS(Circle, Circle);
    NCB_SUBCLASS(Rect, Rect);
    NCB_SUBCLASS(Quad, Quad);
    NCB_SUBCLASS(LayerGetter, LayerGetter);

    // Layer types
    Variant(TJS_W("LayerTypeObj"), (tjs_int)LayerTypeObj);
    Variant(TJS_W("LayerTypeShape"), (tjs_int)LayerTypeShape);
    Variant(TJS_W("LayerTypeLayout"), (tjs_int)LayerTypeLayout);
    Variant(TJS_W("LayerTypeMotion"), (tjs_int)LayerTypeMotion);
    Variant(TJS_W("LayerTypeParticle"), (tjs_int)LayerTypeParticle);
    Variant(TJS_W("LayerTypeCamera"), (tjs_int)LayerTypeCamera);

    // Shape types
    Variant(TJS_W("ShapeTypePoint"), (tjs_int)ShapeTypePoint);
    Variant(TJS_W("ShapeTypeCircle"), (tjs_int)ShapeTypeCircle);
    Variant(TJS_W("ShapeTypeRect"), (tjs_int)ShapeTypeRect);
    Variant(TJS_W("ShapeTypeQuad"), (tjs_int)ShapeTypeQuad);

    // Play flags
    Variant(TJS_W("PlayFlagForce"), (tjs_int)PlayFlagForce);
    Variant(TJS_W("PlayFlagChain"), (tjs_int)PlayFlagChain);
    Variant(TJS_W("PlayFlagAsCan"), (tjs_int)PlayFlagAsCan);
    Variant(TJS_W("PlayFlagJoin"), (tjs_int)PlayFlagJoin);
    Variant(TJS_W("PlayFlagStealth"), (tjs_int)PlayFlagStealth);

    // Transform orders
    Variant(TJS_W("TransformOrderFlip"), (tjs_int)TransformOrderFlip);
    Variant(TJS_W("TransformOrderSlant"), (tjs_int)TransformOrderSlant);
    Variant(TJS_W("TransformOrderZoom"), (tjs_int)TransformOrderZoom);
    Variant(TJS_W("TransformOrderAngle"), (tjs_int)TransformOrderAngle);

    // Coordinate types
    Variant(TJS_W("CoordinateRecutangularXY"),
            (tjs_int)CoordinateRecutangularXY);
    Variant(TJS_W("CoordinateRecutangularXZ"),
            (tjs_int)CoordinateRecutangularXZ);

    Variant("MaskModeStencil", (tjs_int)MaskModeStencil);
    Variant("MaskModeAlpha", (tjs_int)MaskModeAlpha);
}

// ============================================================
// Callbacks (must be under motionplayer.dll module)
// ============================================================

static void PostRegistCallback() {
    iTJSDispatch2 *global = TVPGetScriptDispatch();
    if(!global)
        return;

    auto ensurePlayerClassUseD3DProbe = [](iTJSDispatch2 *playerClass) {
        if(!playerClass) {
            return;
        }
        tTJSVariant marker;
        try {
            TVPExecuteExpression(TJS_W("%[]"), &marker);
        } catch(...) {
            return;
        }
        if(marker.Type() != tvtObject) {
            return;
        }

        // Player_ncb_registerMembers @ 0x6D69C8 registers useD3D as a
        // property object on the Player class; game scripts probe that with
        // typeof Motion.Player.useD3D. This restores the class-level NCB shape
        // without adding a mutable static useD3D state.
        playerClass->PropSet(TJS_MEMBERENSURE | TJS_STATICMEMBER,
                             TJS_W("useD3D"), nullptr, &marker, playerClass);
    };

    // Alias Player class into Motion namespace
    tTJSVariant motionVar;
    if(TJS_SUCCEEDED(
           global->PropGet(0, TJS_W("Motion"), nullptr, &motionVar, global))) {
        iTJSDispatch2 *motion = motionVar.AsObjectNoAddRef();
        if(motion) {
            tTJSVariant playerVar;
            if(TJS_SUCCEEDED(global->PropGet(0, TJS_W("Player"), nullptr,
                                             &playerVar, global))) {
                if(playerVar.Type() == tvtObject &&
                   playerVar.AsObjectNoAddRef() != nullptr) {
                    ensurePlayerClassUseD3DProbe(playerVar.AsObjectNoAddRef());
                    motion->PropSet(TJS_MEMBERENSURE, TJS_W("Player"), nullptr,
                                    &playerVar, motion);
                }
            }
        }
    }

    // Define ShortCutInitialPadKeyMap and related members as empty
    // dictionaries. These are referenced by encrypted keybinder.tjs but may not
    // be defined if the gamepad initialization script hasn't run yet.
    {
        tTJSVariant r;
        try {
            TVPExecuteExpression(
                TJS_W("global.ShortCutInitialPadKeyMap === void "
                      "? (global.ShortCutInitialPadKeyMap = %[]) : void"),
                &r);
            TVPExecuteExpression(
                TJS_W("global.ShortCutInitialGamePadKeyMap === void "
                      "? (global.ShortCutInitialGamePadKeyMap = %[]) : void"),
                &r);
        } catch(...) {
        }
    }

    global->Release();
}

static void PreRegistCallback() {}
static void PostUnregistCallback() {}

NCB_PRE_REGIST_CALLBACK(PreRegistCallback);
NCB_POST_REGIST_CALLBACK(PostRegistCallback);
NCB_POST_UNREGIST_CALLBACK(PostUnregistCallback);

// ============================================================
// emoteplayer.dll module — separate from motionplayer.dll
// In libkrkr2.so, emoteplayer.dll is an independent module whose
// entry callback (sub_682528) loads motionplayer.dll as a dependency,
// then registers EmotePlayer into the Motion namespace.
// ============================================================

NCB_REGISTER_CLASS(D3DEmoteModule) {
    NCB_CONSTRUCTOR(());

    Variant(TJS_W("TimelinePlayFlagParallel"),
            (tjs_int)TimelinePlayFlagParallel);
    Variant(TJS_W("TimelinePlayFlagSequential"),
            (tjs_int)TimelinePlayFlagSequential);

    // Properties
    NCB_PROPERTY(maskMode, getMaskMode, setMaskMode);
    NCB_PROPERTY(maskRegionClipping, getMaskRegionClipping,
                 setMaskRegionClipping);
    NCB_PROPERTY(mipMapEnabled, getMipMapEnabled, setMipMapEnabled);
    NCB_PROPERTY(alphaOp, getAlphaOp, setAlphaOp);
    NCB_PROPERTY(protectTranslucentTextureColor,
                 getProtectTranslucentTextureColor,
                 setProtectTranslucentTextureColor);
    NCB_PROPERTY(pixelateDivision, getPixelateDivision, setPixelateDivision);

    // Methods
    NCB_METHOD(setMaxTextureSize);
}

NCB_REGISTER_CLASS(D3DEmotePlayer) {
    NCB_CONSTRUCTOR((ResourceManager));

    // Properties (same as EmotePlayer subclass, matching IDA registration
    // order)
    NCB_PROPERTY_RO(module, getModule);
    NCB_PROPERTY(completionType, getCompletionType, setCompletionType);
    NCB_PROPERTY(chara, getChara, setChara);
    NCB_PROPERTY(motion, getMotion, setMotion);
    NCB_PROPERTY(motionKey, getMotionKey, setMotionKey);
    NCB_PROPERTY(maskMode, getMaskMode, setMaskMode);
    NCB_PROPERTY(outline, getOutline, setOutline);
    NCB_PROPERTY(priorDraw, getPriorDraw, setPriorDraw);
    NCB_PROPERTY(frameLastTime, getFrameLastTime, setFrameLastTime);
    NCB_PROPERTY(frameLoopTime, getFrameLoopTime, setFrameLoopTime);
    NCB_PROPERTY(loopTime, getLoopTime, setLoopTime);
    NCB_PROPERTY(processedMeshVerticesNum, getProcessedMeshVerticesNum,
                 setProcessedMeshVerticesNum);
    NCB_PROPERTY(visible, getVisible, setVisible);
    NCB_PROPERTY(smoothing, getSmoothing, setSmoothing);
    NCB_PROPERTY(meshDivisionRatio, getMeshDivisionRatio, setMeshDivisionRatio);
    NCB_PROPERTY(queing, getQueuing, setQueuing);
    NCB_PROPERTY(hairScale, getHairScale, setHairScale);
    NCB_PROPERTY(partsScale, getPartsScale, setPartsScale);
    NCB_PROPERTY(bustScale, getBustScale, setBustScale);
    NCB_PROPERTY(bodyScale, getBodyScale, setBodyScale);
    NCB_PROPERTY(useD3D, getUseD3D, setUseD3D);
    NCB_PROPERTY(progress, getProgress, setProgress);
    NCB_PROPERTY(modified, getModified, setModified);
    NCB_PROPERTY(drawvisible, getDrawVisible, setDrawVisible);
    NCB_PROPERTY(drawOpacity, getDrawOpacity, setDrawOpacity);
    NCB_PROPERTY(opengl, getOpengl, setOpengl);
    NCB_PROPERTY_RO(animating, getAnimating);
    NCB_PROPERTY_RO(playCallback, getPlayCallback);

    // Methods
    NCB_METHOD(create);
    NCB_METHOD(load);
    NCB_METHOD(clone);
    NCB_METHOD(serialize);
    NCB_METHOD(unserialize);
    NCB_METHOD(show);
    NCB_METHOD(hide);
    NCB_METHOD(assignState);
    NCB_METHOD(initPhysics);
    NCB_METHOD_RAW_CALLBACK(setRot, &EmotePlayer::setRotCompat, 0);
    NCB_METHOD_RAW_CALLBACK(setRotate, &EmotePlayer::setRotCompat, 0);
    NCB_METHOD(getRot);
    NCB_METHOD_RAW_CALLBACK(setCoord, &EmotePlayer::setCoordCompat, 0);
    NCB_METHOD_RAW_CALLBACK(setScale, &EmotePlayer::setScaleCompat, 0);
    NCB_METHOD(getScale);
    NCB_METHOD(setMirror);
    NCB_METHOD_RAW_CALLBACK(setColor, &EmotePlayer::setColorCompat, 0);
    NCB_METHOD(getColor);
    NCB_METHOD(countVariables);
    NCB_METHOD(getVariableLabelAt);
    NCB_METHOD(countVariableFrameAt);
    NCB_METHOD(getVariableFrameLabelAt);
    NCB_METHOD(getVariableFrameValueAt);
    NCB_METHOD_RAW_CALLBACK(setVariable, &EmotePlayer::setVariableCompat, 0);
    NCB_METHOD(getVariable);
    NCB_METHOD(getVariableFrameList);
    NCB_METHOD_RAW_CALLBACK(startWind, &EmotePlayer::startWindCompat, 0);
    NCB_METHOD_RAW_CALLBACK(stopWind, &EmotePlayer::stopWindCompat, 0);
    NCB_METHOD(countMainTimelines);
    NCB_METHOD(getMainTimelineLabelAt);
    NCB_METHOD(countDiffTimelines);
    NCB_METHOD(getDiffTimelineLabelAt);
    NCB_METHOD(getDiffTimelineLabelList);
    NCB_METHOD(countPlayingTimelines);
    NCB_METHOD(getPlayingTimelineLabelAt);
    NCB_METHOD(getPlayingTimelineFlagsAt);
    NCB_METHOD(isLoopTimeline);
    NCB_METHOD(getLoopTimeline);
    NCB_METHOD(getTimelineTotalFrameCount);
    NCB_METHOD(play);
    NCB_METHOD(playTimeline);
    NCB_METHOD(isTimelinePlaying);
    NCB_METHOD(getTimelinePlaying);
    NCB_METHOD(stopTimeline);
    NCB_METHOD(setTimeline);
    NCB_METHOD(setTimelineBlendRatio);
    NCB_METHOD(getTimelineBlendRatio);
    NCB_METHOD(fadeInTimeline);
    NCB_METHOD(fadeOutTimeline);
    NCB_METHOD(getPlayingTimelineInfoList);
    NCB_METHOD(skip);
    NCB_METHOD(skipToSync);
    NCB_METHOD(addPlayCallback);
    NCB_METHOD(pass);
    NCB_METHOD(progress);
    NCB_METHOD_DETAIL(draw, Class, void, Class::draw, (tTJSVariant));
    NCB_METHOD_RAW_CALLBACK(setDrawAffineTranslateMatrix,
                            &EmotePlayer::setDrawAffineTranslateMatrixCompat,
                            0);
    NCB_METHOD_RAW_CALLBACK(setOuterForce, &EmotePlayer::setOuterForceCompat,
                            0);
    NCB_METHOD(getOuterForce);
    NCB_METHOD_RAW_CALLBACK(contains, &EmotePlayer::containsCompat, 0);
}
