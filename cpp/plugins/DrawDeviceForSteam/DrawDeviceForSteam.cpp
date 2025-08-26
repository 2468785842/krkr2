#define NCB_MODULE_NAME TJS_W("DrawDeviceForSteam.dll")

#include "ncbind.hpp"
#include "tjs.h"
#include "DrawDevice.h"
#include "visual/WindowIntf.h"
// #include "simplebinder/simplebinder.hpp"
// using SimpleBinder::BindUtil;

typedef unsigned long ULONG;

//-------------------------
// 1. 壳实现
//-------------------------
class DrawDeviceForSteam : public iTVPDrawDevice {
    iTVPDrawDevice *Real;
    ULONG RefCount = 1;
    static iTVPWindow *interface;

public:
    tjs_int64 Getinterface() {

        return reinterpret_cast<tjs_int64>(TVPMainWindow->GetDrawDevice());
    }
    iTVPDrawDevice *GetReal() { return Real; }
    ~DrawDeviceForSteam() {}

    DrawDeviceForSteam() { Real = TVPMainWindow->GetDrawDevice(); }

    static tjs_error factory(DrawDeviceForSteam **result, tjs_int numparams,
                             tTJSVariant **params, iTJSDispatch2 *objthis) {
        *result = new DrawDeviceForSteam();
        return TJS_S_OK;
    }

    //--------------------------------------------------
    // 以下所有纯虚函数全部透传
    //--------------------------------------------------
    void Destruct() override {
        if(Real)
            Real->Destruct();
    }

    void SetWindowInterface(iTVPWindow *window) override {
        if(Real)
            Real->SetWindowInterface(window);
    }

    void AddLayerManager(iTVPLayerManager *manager) override {
        if(Real)
            Real->AddLayerManager(manager);
    }

    void RemoveLayerManager(iTVPLayerManager *manager) override {
        if(Real)
            Real->RemoveLayerManager(manager);
    }

    void SetDestRectangle(const tTVPRect &rect) override {
        if(Real)
            Real->SetDestRectangle(rect);
    }

    void SetWindowSize(tjs_int w, tjs_int h) override {
        if(Real)
            Real->SetWindowSize(w, h);
    }

    void SetClipRectangle(const tTVPRect &rect) override {
        if(Real)
            Real->SetClipRectangle(rect);
    }

    void GetSrcSize(tjs_int &w, tjs_int &h) override {
        if(Real)
            Real->GetSrcSize(w, h);
    }

    void NotifyLayerResize(iTVPLayerManager *manager) override {
        if(Real)
            Real->NotifyLayerResize(manager);
    }

    void NotifyLayerImageChange(iTVPLayerManager *manager) override {
        if(Real)
            Real->NotifyLayerImageChange(manager);
    }

    void OnClick(tjs_int x, tjs_int y) override {
        if(Real)
            Real->OnClick(x, y);
    }

    void OnDoubleClick(tjs_int x, tjs_int y) override {
        if(Real)
            Real->OnDoubleClick(x, y);
    }

    void OnMouseDown(tjs_int x, tjs_int y, tTVPMouseButton mb,
                     tjs_uint32 flags) override {
        if(Real)
            Real->OnMouseDown(x, y, mb, flags);
    }

    void OnMouseUp(tjs_int x, tjs_int y, tTVPMouseButton mb,
                   tjs_uint32 flags) override {
        if(Real)
            Real->OnMouseUp(x, y, mb, flags);
    }

    void OnMouseMove(tjs_int x, tjs_int y, tjs_uint32 flags) override {
        if(Real)
            Real->OnMouseMove(x, y, flags);
    }

    void OnReleaseCapture() override {
        if(Real)
            Real->OnReleaseCapture();
    }

    void OnMouseOutOfWindow() override {
        if(Real)
            Real->OnMouseOutOfWindow();
    }

    void OnKeyDown(tjs_uint key, tjs_uint32 shift) override {
        if(Real)
            Real->OnKeyDown(key, shift);
    }

    void OnKeyUp(tjs_uint key, tjs_uint32 shift) override {
        if(Real)
            Real->OnKeyUp(key, shift);
    }

    void OnKeyPress(tjs_char key) override {
        if(Real)
            Real->OnKeyPress(key);
    }

    void OnMouseWheel(tjs_uint32 shift, tjs_int delta, tjs_int x,
                      tjs_int y) override {
        if(Real)
            Real->OnMouseWheel(shift, delta, x, y);
    }

    void OnTouchDown(tjs_real x, tjs_real y, tjs_real cx, tjs_real cy,
                     tjs_uint32 id) override {
        if(Real)
            Real->OnTouchDown(x, y, cx, cy, id);
    }

    void OnTouchUp(tjs_real x, tjs_real y, tjs_real cx, tjs_real cy,
                   tjs_uint32 id) override {
        if(Real)
            Real->OnTouchUp(x, y, cx, cy, id);
    }

    void OnTouchMove(tjs_real x, tjs_real y, tjs_real cx, tjs_real cy,
                     tjs_uint32 id) override {
        if(Real)
            Real->OnTouchMove(x, y, cx, cy, id);
    }

    void OnTouchScaling(tjs_real startdist, tjs_real curdist, tjs_real cx,
                        tjs_real cy, tjs_int flag) override {
        if(Real)
            Real->OnTouchScaling(startdist, curdist, cx, cy, flag);
    }

    void OnTouchRotate(tjs_real startangle, tjs_real curangle, tjs_real dist,
                       tjs_real cx, tjs_real cy, tjs_int flag) override {
        if(Real)
            Real->OnTouchRotate(startangle, curangle, dist, cx, cy, flag);
    }

    void OnMultiTouch() override {
        if(Real)
            Real->OnMultiTouch();
    }

    void OnDisplayRotate(tjs_int orientation, tjs_int rotate, tjs_int bpp,
                         tjs_int width, tjs_int height) override {
        if(Real)
            Real->OnDisplayRotate(orientation, rotate, bpp, width, height);
    }

    void RecheckInputState() override {
        if(Real)
            Real->RecheckInputState();
    }

    void SetDefaultMouseCursor(iTVPLayerManager *manager) override {
        if(Real)
            Real->SetDefaultMouseCursor(manager);
    }

    void SetMouseCursor(iTVPLayerManager *manager, tjs_int cursor) override {
        if(Real)
            Real->SetMouseCursor(manager, cursor);
    }

    void GetCursorPos(iTVPLayerManager *manager, tjs_int &x,
                      tjs_int &y) override {
        if(Real)
            Real->GetCursorPos(manager, x, y);
    }

    void SetCursorPos(iTVPLayerManager *manager, tjs_int x,
                      tjs_int y) override {
        if(Real)
            Real->SetCursorPos(manager, x, y);
    }

    void WindowReleaseCapture(iTVPLayerManager *manager) override {
        if(Real)
            Real->WindowReleaseCapture(manager);
    }

    void SetHintText(iTVPLayerManager *manager, iTJSDispatch2 *sender,
                     const ttstr &text) override {
        if(Real)
            Real->SetHintText(manager, sender, text);
    }

    void SetAttentionPoint(iTVPLayerManager *manager, tTJSNI_BaseLayer *layer,
                           tjs_int l, tjs_int t) override {
        if(Real)
            Real->SetAttentionPoint(manager, layer, l, t);
    }

    void DisableAttentionPoint(iTVPLayerManager *manager) override {
        if(Real)
            Real->DisableAttentionPoint(manager);
    }

    void SetImeMode(iTVPLayerManager *manager, tTVPImeMode mode) override {
        if(Real)
            Real->SetImeMode(manager, mode);
    }

    void ResetImeMode(iTVPLayerManager *manager) override {
        if(Real)
            Real->ResetImeMode(manager);
    }

    tTJSNI_BaseLayer *GetPrimaryLayer() override {
        return Real ? Real->GetPrimaryLayer() : nullptr;
    }

    tTJSNI_BaseLayer *GetFocusedLayer() override {
        return Real ? Real->GetFocusedLayer() : nullptr;
    }

    void SetFocusedLayer(tTJSNI_BaseLayer *layer) override {
        if(Real)
            Real->SetFocusedLayer(layer);
    }

    void RequestInvalidation(const tTVPRect &rect) override {
        if(Real)
            Real->RequestInvalidation(rect);
    }

    void Update() override {
        if(Real)
            Real->Update();
    }

    void Show() override {
        if(Real)
            Real->Show();
    }

    void StartBitmapCompletion(iTVPLayerManager *manager) override {
        if(Real)
            Real->StartBitmapCompletion(manager);
    }

    void NotifyBitmapCompleted(iTVPLayerManager *manager, tjs_int x, tjs_int y,
                               tTVPBaseTexture *bmp, const tTVPRect &cliprect,
                               tTVPLayerType type, tjs_int opacity) override {
        if(Real)
            Real->NotifyBitmapCompleted(manager, x, y, bmp, cliprect, type,
                                        opacity);
    }

    void EndBitmapCompletion(iTVPLayerManager *manager) override {
        if(Real)
            Real->EndBitmapCompletion(manager);
    }

    void DumpLayerStructure() override {
        if(Real)
            Real->DumpLayerStructure();
    }

    void SetShowUpdateRect(bool b) override {
        if(Real)
            Real->SetShowUpdateRect(b);
    }

    void Clear() override {
        if(Real)
            Real->Clear();
    }

    bool SwitchToFullScreen(int window, tjs_uint w, tjs_uint h, tjs_uint bpp,
                            tjs_uint color, bool changeresolution) override {
        return Real ? Real->SwitchToFullScreen(window, w, h, bpp, color,
                                               changeresolution)
                    : false;
    }

    void RevertFromFullScreen(int window, tjs_uint w, tjs_uint h, tjs_uint bpp,
                              tjs_uint color) override {
        if(Real)
            Real->RevertFromFullScreen(window, w, h, bpp, color);
    }

    bool WaitForVBlank(tjs_int *in_vblank, tjs_int *delayed) override {
        return Real ? Real->WaitForVBlank(in_vblank, delayed) : false;
    }
};

//----------------------------------
// 2. 工厂函数：new 一个壳并返回
//----------------------------------
tjs_error
CreateDrawDeviceForSteam(iTJSDispatch2 *objthis, // 会被 simplebinder 传进来
                         DrawDeviceForSteam *&out, // 输出实例指针
                         tTJSVariant **, tjs_int, // 可变参数（这里不用）
                         tjs_int, tTJSVariant **) {
    if(out) {
        TVPMainWindow->DrawDevice = out;
        return TJS_S_OK;
    } else {
        return TJS_S_FALSE;
    }
}

//----------------------------------
// 3. 析构函数：析构时恢复原设备（可选）
//----------------------------------
tjs_error
DestroyDrawDeviceForSteam(iTJSDispatch2 *objthis, // 会被 simplebinder 传进来
                          DrawDeviceForSteam *&p, // 输出实例指针
                          tTJSVariant **, tjs_int, // 可变参数（这里不用）
                          tjs_int, tTJSVariant **) {
    if(p) {
        // 把窗口 DrawDevice 恢复成原来的
        TVPMainWindow->DrawDevice = p->GetReal(); // 你壳里留个 getter 就行
    }
    delete p;
    return TJS_S_OK;
}

class tTJSNI_DrawDeviceForSteam : public tTJSNativeInstance {
private:
    DrawDeviceForSteam *CppInstance; // 持有 C++ 对象指针
public:
    tTJSNI_DrawDeviceForSteam() = default;
    tjs_error Construct(tjs_int, tTJSVariant **, iTJSDispatch2 *) override {
        if(CppInstance)
            delete CppInstance; // 如果之前已经创建，先清理 (不太可能在这里发生)
        CppInstance = new DrawDeviceForSteam(); // 创建 C++ 对象
        TVPMainWindow->DrawDevice = CppInstance; // 如果你还想替换全局的

        // 将 C++ 实例与 tTJSNativeInstance 关联 (对于 ncbind，这通常在
        // CreateNativeInstance 中完成) SetNativeInstance(CppInstance); // 假设
        // ncbind 提供了这样的机制或 tTJSNI_Skel 做了这个
        return TJS_S_OK;
    }
    void Invalidate() override {

        if(CppInstance) {
            // 如果 TVPMainWindow->DrawDevice 指向的是这个 CppInstance，
            // 你可能需要在这里恢复它或者采取其他措施。
            if(TVPMainWindow->DrawDevice == CppInstance)
                TVPMainWindow->DrawDevice = CppInstance->GetReal();
            delete CppInstance;
            CppInstance = nullptr;
        }
    }
};

class tTJSNC_DrawDeviceForSteam : public tTJSNativeClass {
public:
    tTJSNC_DrawDeviceForSteam() :
        tTJSNativeClass(TJS_W("DrawDeviceForSteam")) {}
    static tjs_uint32 ClassID;

private:
    iTJSNativeInstance *CreateNativeInstance() override {
        return new tTJSNI_DrawDeviceForSteam();
    }
};
tjs_uint32 tTJSNC_DrawDeviceForSteam::ClassID = -1;

// 工厂函数：脚本 new DrawDeviceForSteam() 时执行
tjs_error
CreateDrawDeviceForSteam(iTJSDispatch2 * /*objthis*/,
                         DrawDeviceForSteam *&out, // 返回给 TJS 的对象
                         tTJSVariant **, tjs_int) // 可变参数占位
{

    out = new DrawDeviceForSteam(); // 创建透传壳
    TVPMainWindow->DrawDevice = out; // 立即替换
    return TJS_S_OK;
}

NCB_REGISTER_CLASS(DrawDeviceForSteam) {
    Factory(&DrawDeviceForSteam::factory);
    Property(TJS_W("interface"), &DrawDeviceForSteam::Getinterface, int());
}
