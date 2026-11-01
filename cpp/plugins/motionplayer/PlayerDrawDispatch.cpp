// PlayerDrawDispatch.cpp — draw raw callback dispatch and draw target routing
// Split out for maintainability.
//
#include "PlayerRenderInternal.h"
#include "MotionTraceWeb.h"
#include "ncbind.hpp"

using namespace motion::internal;

namespace motion {
    tjs_error Player::setDrawAffineTranslateMatrixCompat(
        tTJSVariant *result, tjs_int numparams, tTJSVariant **param,
        Player *nativeInstance) {
        if(result) {
            result->Clear();
        }
        if(!nativeInstance) {
            return TJS_E_INVALIDOBJECT;
        }

        std::array<double, 6> matrix{ 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 };
        if(numparams >= 6) {
            for(size_t index = 0; index < matrix.size(); ++index) {
                if(!param[index] || param[index]->Type() == tvtVoid) {
                    return TJS_E_INVALIDPARAM;
                }
                matrix[index] = param[index]->AsReal();
            }
        } else if(numparams == 1 && param[0] && param[0]->Type() == tvtObject &&
                  param[0]->AsObjectNoAddRef() != nullptr) {
            const auto object = *param[0];
            tTJSVariant value;
            if(getObjectProperty(object, TJS_W("m11"), value) &&
               value.Type() != tvtVoid) {
                matrix[0] = value.AsReal();
            }
            if(getObjectProperty(object, TJS_W("m21"), value) &&
               value.Type() != tvtVoid) {
                matrix[1] = value.AsReal();
            }
            if(getObjectProperty(object, TJS_W("m12"), value) &&
               value.Type() != tvtVoid) {
                matrix[2] = value.AsReal();
            }
            if(getObjectProperty(object, TJS_W("m22"), value) &&
               value.Type() != tvtVoid) {
                matrix[3] = value.AsReal();
            }
            if(getObjectProperty(object, TJS_W("m14"), value) &&
               value.Type() != tvtVoid) {
                matrix[4] = value.AsReal();
            }
            if(getObjectProperty(object, TJS_W("m24"), value) &&
               value.Type() != tvtVoid) {
                matrix[5] = value.AsReal();
            }
        } else {
            return TJS_E_BADPARAMCOUNT;
        }

        nativeInstance->_runtime->drawAffineMatrix = matrix;
        const auto motionPath =
            nativeInstance->_runtime && nativeInstance->_runtime->activeMotion
            ? nativeInstance->_runtime->activeMotion->path
            : std::string{};
        const bool isIdentity = matrix[0] == 1.0 && matrix[1] == 0.0 &&
            matrix[2] == 0.0 && matrix[3] == 1.0 && matrix[4] == 0.0 &&
            matrix[5] == 0.0;
        detail::logoChainTraceLogf(
            motionPath, "setDrawAffine", "0x6D4F14",
            nativeInstance->_clampedEvalTime,
            "numparams={} matrix=[{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f}] "
            "nonIdentityFlag={} routeSource={}",
            numparams, matrix[0], matrix[1], matrix[2], matrix[3], matrix[4],
            matrix[5], isIdentity ? 0 : 1,
            (numparams >= 6)
                ? "six-params"
                : ((numparams == 1) ? "matrix-object" : "invalid"));
        return TJS_S_OK;
    }

    tjs_error Player::captureCanvasCompat(tTJSVariant *result,
                                          tjs_int numparams,
                                          tTJSVariant **param,
                                          Player *nativeInstance) {
        if(result) {
            result->Clear();
        }
        if(!nativeInstance) {
            return TJS_E_INVALIDOBJECT;
        }

        if(numparams > 0 && param[0] && param[0]->Type() == tvtObject &&
           param[0]->AsObjectNoAddRef() != nullptr) {
            if(nativeInstance->renderToLayer(param[0]->AsObjectNoAddRef())) {
                if(result) {
                    *result = *param[0];
                }
                return TJS_S_OK;
            }
        }

        if(result) {
            *result = nativeInstance->captureCanvas();
        }
        return TJS_S_OK;
    }

    void Player::draw(tTJSVariant target) {
        // Aligned to libkrkr2.so Player_draw_NCBWrapper @ 0x681900:
        // the wrapper receives one TJS argument by value and forwards a local
        // variant copy into Player_drawCompat @ 0x6D5FB8.
        drawCompat(&target);
    }

    // drawCompat — aligned to libkrkr2.so Player_drawCompat @ 0x6D5FB8 /
    // Player_drawD3D @ 0x6D5B90. This is the native helper below the NCB
    // wrapper; TJS result/numparams/objthis are intentionally not part of this
    // boundary.
    void Player::drawCompat(tTJSVariant *arg) {
        const auto motionPath = _runtime && _runtime->activeMotion
            ? _runtime->activeMotion->path
            : std::string{};
        iTJSDispatch2 *paramObj = (arg && arg->Type() == tvtObject)
            ? arg->AsObjectNoAddRef()
            : nullptr;
#if defined(KRKR2_WASMTIME_HEADLESS)
        detail::MotionTraceRenderDrawScope renderTrace(this, arg, paramObj);
#endif
        const auto logDrawMatrix = [&](const char *route) {
            if(!_runtime) {
                return;
            }
            detail::logoChainTraceLogf(
                motionPath, "drawCompat.matrix", "0x6D5FB8", _clampedEvalTime,
                "route={} "
                "drawAffine=[{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f}] "
                "cameraOffset=({:.3f},{:.3f}) "
                "sampleExpectedYuzu=[1,0,0,1,960,540]",
                route ? route : "", _runtime->drawAffineMatrix[0],
                _runtime->drawAffineMatrix[1], _runtime->drawAffineMatrix[2],
                _runtime->drawAffineMatrix[3], _runtime->drawAffineMatrix[4],
                _runtime->drawAffineMatrix[5], _cameraOffsetX, _cameraOffsetY);
        };

        if(!paramObj) {
#if defined(KRKR2_WASMTIME_HEADLESS)
            renderTrace.setRoute("no_target");
#endif
            detail::logoChainTraceLogf(motionPath, "drawCompat.dispatch",
                                       "0x6D5FB8", _clampedEvalTime,
                                       "route=no-param");
            return;
        }

        // Step 1: Check if param is D3DAdaptor (libkrkr2.so checks NIS with
        // D3DAdaptor classID). If so, set _d3dDrawMode and render immediately.
        {
            auto *d3dAdaptor =
                ncbInstanceAdaptor<D3DAdaptor>::GetNativeInstance(paramObj,
                                                                  false);
            if(d3dAdaptor) {
#if defined(KRKR2_WASMTIME_HEADLESS)
                renderTrace.recordTargetCheckD3D(true);
                renderTrace.setRoute("d3d_adaptor");
#endif
                detail::logoChainTraceCheck(
                    motionPath, "drawCompat.dispatch", "0x6D5FB8",
                    _clampedEvalTime, "D3DAdaptor -> Player_drawD3D",
                    "D3DAdaptor -> Player_drawD3D", true,
                    "drawCompat D3D routing mismatch");
                logDrawMatrix("d3d");
                _d3dDrawMode = true;
                renderToD3DAdaptor(d3dAdaptor);
                return;
            }
#if defined(KRKR2_WASMTIME_HEADLESS)
            renderTrace.recordTargetCheckD3D(false);
#endif
        }

        // Step 2: Check if param is SLA.
        // Aligned to libkrkr2.so Player_drawCompat (0x6D5FB8):
        // the native code only checks the SeparateLayerAdaptor class ID here.
        // It does not route plain Layer objects through the SLA backend just
        // because they resolve to an owner/target layer.
        {
            auto *sla =
                ncbInstanceAdaptor<SeparateLayerAdaptor>::GetNativeInstance(
                    paramObj, false);
            if(sla) {
#if defined(KRKR2_WASMTIME_HEADLESS)
                renderTrace.recordTargetCheckSLA(true);
                renderTrace.setRoute("separate_layer_adaptor");
#endif
                detail::logoChainTraceCheck(
                    motionPath, "drawCompat.dispatch", "0x6D5FB8",
                    _clampedEvalTime, "SeparateLayerAdaptor -> Player_DrawSLA",
                    "SeparateLayerAdaptor -> Player_DrawSLA", true,
                    "drawCompat SLA routing mismatch");
                logDrawMatrix("sla");
                renderToSeparateLayerAdaptor(paramObj);
                return;
            }
#if defined(KRKR2_WASMTIME_HEADLESS)
            renderTrace.recordTargetCheckSLA(false);
#endif
        }

        // Step 3: ordinary render-list path. Android does not dispatch plain
        // Layer objects at the top of Player_drawCompat @ 0x6D5FB8; it builds
        // render items first, then branches on d3dDrawMode and only then hands
        // a copied target variant to Player_renderToCanvas_guess @ 0x6C7440.
        ensureMotionLoaded();
        if(!_runtime || !_runtime->activeMotion) {
#if defined(KRKR2_WASMTIME_HEADLESS)
            renderTrace.setRoute("no_motion");
#endif
            detail::logoChainTraceLogf(motionPath, "drawCompat.dispatch",
                                       "0x6D5FB8", _clampedEvalTime,
                                       "route=no-motion");
            return;
        }

        const bool prepareOk = prepareRenderItems();
#if defined(KRKR2_WASMTIME_HEADLESS)
        renderTrace.recordPrepareResult(prepareOk);
#endif
        if(!prepareOk) {
#if defined(KRKR2_WASMTIME_HEADLESS)
            renderTrace.setRoute("prepare_empty");
#endif
            G_PluginLog.warn(
                "Player::drawCompat: prepareRenderItems returned empty for "
                "path={} nodes={}",
                motionPath, _runtime ? _runtime->nodes.size() : 0);
            detail::logoChainTraceCheck(
                motionPath, "drawCompat.dispatch", "0x6D5FB8", _clampedEvalTime,
                "prepareRenderItems should produce a render list",
                "prepareRenderItems returned false", false,
                "drawCompat ordinary path stopped before renderToCanvas");
            return;
        }

#if defined(KRKR2_WASMTIME_HEADLESS)
        renderTrace.recordBranchAfterPrepare(_d3dDrawMode);
#endif
        if(_d3dDrawMode) {
            const bool ok = renderViaSharedD3DAdaptor(paramObj);
#if defined(KRKR2_WASMTIME_HEADLESS)
            renderTrace.setRoute(ok ? "shared_d3d_after_prepare" : "failed");
#endif
            detail::logoChainTraceCheck(
                motionPath, "drawCompat.dispatch", "0x6D5FB8", _clampedEvalTime,
                "prepareRenderItems -> shared D3D render path",
                ok ? "shared_d3d" : "shared_d3d_failed", ok,
                "drawCompat shared D3D path failed");
            logDrawMatrix(ok ? "shared_d3d" : "shared_d3d_failed");
            return;
        }

        applyPreparedRenderItemTranslateOffsets();
#if defined(KRKR2_WASMTIME_HEADLESS)
        renderTrace.recordApplyTranslateOffset();
#endif
        tTJSVariant targetCopy;
        targetCopy = *arg;
        const bool rendered = renderToCanvasLike_0x6C7440(&targetCopy, true);
#if defined(KRKR2_WASMTIME_HEADLESS)
        renderTrace.recordRenderToCanvas(rendered);
#endif
        const bool internalAssignRequested =
            rendered && _needsInternalAssignImages;
        const bool updated = rendered && updateLayerAfterDrawLike_0x6CE7D8(arg);
#if defined(KRKR2_WASMTIME_HEADLESS)
        if(rendered) {
            renderTrace.recordUpdateLayerAfterDraw(internalAssignRequested,
                                                   updated);
        }
#endif
        const bool ok = rendered && updated;
#if defined(KRKR2_WASMTIME_HEADLESS)
        renderTrace.setRoute(ok ? "ordinary_layer" : "failed");
#endif
        detail::logoChainTraceCheck(
            motionPath, "drawCompat.dispatch", "0x6D5FB8", _clampedEvalTime,
            "prepareRenderItems -> applyTranslateOffset -> "
            "renderToCanvas(copy(target)) -> updateLayerAfterDraw(target)",
            ok ? "render_to_canvas" : "render_to_canvas_failed", ok,
            "drawCompat ordinary render path failed");
        logDrawMatrix(ok ? "render_to_canvas" : "render_to_canvas_failed");
    }

} // namespace motion
