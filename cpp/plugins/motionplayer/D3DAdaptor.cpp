// D3DAdaptor.cpp — Motion.D3DAdaptor texture target implementation.
//
#include "D3DAdaptor.h"

#include <algorithm>
#include <cstring>

#include "TVPLog.h"

#include "MsgIntf.h"
#include "RenderManager.h"

namespace motion {

    D3DAdaptor::~D3DAdaptor() { releaseTargetTexture(); }

    tjs_error D3DAdaptor::factory(D3DAdaptor **result, tjs_int numparams,
                                  tTJSVariant **param, iTJSDispatch2 *) {
        G_PluginLog.warn("D3DAdaptor::factory called, numparams={}", numparams);
        if(numparams < 5)
            return TJS_E_BADPARAMCOUNT;
        if(!result)
            return TJS_E_INVALIDPARAM;
        if(!param || !param[0])
            return TJS_E_INVALIDPARAM;

        iTJSDispatch2 *windowObject = param[0]->AsObjectNoAddRef();
        if(!windowObject ||
           windowObject->IsInstanceOf(0, nullptr, nullptr, TJS_W("Window"),
                                      windowObject) != TJS_S_TRUE) {
            TVPThrowExceptionMessage(TJS_W("must set Window object"));
        }

        auto *obj = new D3DAdaptor();
        obj->initializeLike_0x6ADB10(*param[0],
                                     static_cast<int>(param[1]->AsInteger()),
                                     static_cast<int>(param[2]->AsInteger()),
                                     static_cast<int>(param[3]->AsInteger()),
                                     static_cast<int>(param[4]->AsInteger()));
        G_PluginLog.warn("D3DAdaptor::factory OK, w={} h={} center=({}, {})",
                         obj->_width, obj->_height, obj->_centerX,
                         obj->_centerY);
        *result = obj;
        return TJS_S_OK;
    }

    void D3DAdaptor::setSize(int w, int h) {
        if(_width == w && _height == h) {
            return;
        }
        _width = w;
        _height = h;
        releaseTargetTexture();
        allocBuffer();
        ensureTargetTexture();
    }

    tjs_error D3DAdaptor::captureCanvas(tTJSVariant *result, tjs_int numparams,
                                        tTJSVariant **param,
                                        iTJSDispatch2 *objthis) {
        (void)objthis;
        if(numparams < 1 || !param[0])
            return TJS_E_BADPARAMCOUNT;

        iTJSDispatch2 *layerObj = param[0]->AsObjectNoAddRef();
        if(!layerObj)
            return TJS_E_INVALIDPARAM;

        tTJSNI_BaseLayer *layer = nullptr;
        if(TJS_FAILED(layerObj->NativeInstanceSupport(
               TJS_NIS_GETINSTANCE, tTJSNC_Layer::ClassID,
               reinterpret_cast<iTJSNativeInstance **>(&layer))) ||
           !layer) {
            return TJS_E_INVALIDPARAM;
        }

        if(_width <= 0 || _height <= 0) {
            return TJS_S_OK;
        }

        if(!layer->GetHasImage())
            layer->SetHasImage(true);
        layer->SetImageSize(static_cast<tjs_uint>(_width),
                            static_cast<tjs_uint>(_height));

        auto *dst = reinterpret_cast<std::uint8_t *>(
            layer->GetMainImagePixelBufferForWrite());
        auto dstPitch = layer->GetMainImagePixelBufferPitch();
        if(!dst || dstPitch <= 0)
            return TJS_S_OK;

        copyTargetTextureRowsForCaptureLike_0x6AD92C(dst, dstPitch);

        layer->Update(false);

        if(result)
            *result = *param[0];
        return TJS_S_OK;
    }

    tjs_error D3DAdaptor::captureCanvasStatic(tTJSVariant *result,
                                              tjs_int numparams,
                                              tTJSVariant **param,
                                              D3DAdaptor *nativeInstance) {
        if(!nativeInstance)
            return TJS_E_NATIVECLASSCRASH;
        return nativeInstance->captureCanvas(result, numparams, param, nullptr);
    }

    iTJSDispatch2 *D3DAdaptor::getWindowObject() const {
        return _window.Type() == tvtObject ? _window.AsObjectNoAddRef()
                                           : nullptr;
    }

    bool D3DAdaptor::ensureTargetTexture() {
        if(_targetTexture) {
            return true;
        }
        if(_width <= 0 || _height <= 0) {
            return false;
        }
        // Mirrors D3DAdaptor_init @ 0x6ADB10: create adaptor+48 as an RGBA
        // render texture sized from constructor width/height.
        _targetTexture = TVPGetRenderManager()->CreateTexture2D(
            nullptr, 0, static_cast<unsigned int>(_width),
            static_cast<unsigned int>(_height), TVPTextureFormat::RGBA, 0);
        return _targetTexture != nullptr;
    }

    void D3DAdaptor::clearTargetTexture() {
        if(!_targetTexture || _width <= 0 || _height <= 0) {
            clearBuffer();
            return;
        }

        auto *mgr = TVPGetRenderManager();
        auto *method = mgr->GetRenderMethod("FillARGB");
        if(!method) {
            clearBuffer();
            return;
        }
        const int colorId = method->EnumParameterID("color");
        method->SetParameterColor4B(colorId,
                                    static_cast<unsigned int>(_clearColor));
        const tTVPRect rc(0, 0, _width, _height);
        mgr->OperateRect(method, _targetTexture, nullptr, rc,
                         tRenderTexRectArray());
    }

    bool
    D3DAdaptor::copyTargetTextureRowsForCaptureLike_0x6AD92C(std::uint8_t *dst,
                                                             tjs_int dstPitch) {
        if(!dst || dstPitch <= 0 || _width <= 0 || _height <= 0) {
            return false;
        }

        const std::uint8_t *srcBase = nullptr;
        tjs_int srcPitch = 0;
        int copyWidth = _width;
        int copyHeight = _height;

        // Aligned to libkrkr2.so D3DAdaptor_captureCanvas @ 0x6AD92C: the
        // primary path reads back adaptor+48 texture rows into the target
        // Layer.
        if(_targetTexture) {
            srcBase = static_cast<const std::uint8_t *>(
                _targetTexture->GetScanLineForRead(0));
            srcPitch = _targetTexture->GetPitch();
            copyWidth = std::min<int>(_width, _targetTexture->GetWidth());
            copyHeight = std::min<int>(_height, _targetTexture->GetHeight());
        }

        if(!srcBase && !_buffer.empty()) {
            srcBase = _buffer.data();
            srcPitch = static_cast<tjs_int>(_width * 4);
        }
        if(!srcBase || srcPitch <= 0) {
            return false;
        }

        const int rowBytes = std::min<int>(copyWidth * 4, dstPitch);
        for(int y = 0; y < copyHeight; ++y) {
            std::memcpy(dst + dstPitch * y, srcBase + srcPitch * y,
                        static_cast<size_t>(rowBytes));
        }
        return true;
    }

    void D3DAdaptor::initializeLike_0x6ADB10(const tTJSVariant &window,
                                             int width, int height, int centerX,
                                             int centerY) {
        _window = window;
        _width = width;
        _height = height;
        _centerX = centerX;
        _centerY = centerY;
        allocBuffer();
        ensureTargetTexture();
    }

    void D3DAdaptor::clearBuffer() {
        if(!_buffer.empty()) {
            std::memset(_buffer.data(), 0, _buffer.size());
        }
    }

    void D3DAdaptor::allocBuffer() {
        if(_width > 0 && _height > 0) {
            _buffer.resize(static_cast<size_t>(_width) *
                               static_cast<size_t>(_height) * 4u,
                           0);
        } else {
            _buffer.clear();
        }
    }

    void D3DAdaptor::releaseTargetTexture() {
        if(_targetTexture) {
            _targetTexture->Release();
            _targetTexture = nullptr;
        }
    }

} // namespace motion
