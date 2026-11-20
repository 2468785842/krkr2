#pragma once

//---------------------------------------------------------------------------
// Pure C++ iWindowLayer implementation for the SDL3 shell, replacing the
// ScrollView-based TVPWindowLayer (environ/impl/TVPWindow.h).
//
// What lives here:
//  - geometry bookkeeping for position/size/visibility/Z-order/zoom, minus
//    any scenegraph (the SDL shell has none yet);
//  - the RecalcPaintBox letterbox algorithm, ported from TVPWindow.cpp:562-586;
//  - touch -> mouse synthesis through krkr::input::TouchButton
//    (TouchButton.h:13-21, one/two/three fingers -> left/right/middle);
//  - paint-box coordinate conversion for pointer input (the role of
//    SDL_RenderCoordinatesFromWindow in the prototype).
//
// What does NOT live here: any rendering. Z-ordering, layer textures and draw
// are owned by the B3 compositor; every gap is tagged "B3" in the bodies.
// No cocos headers are referenced.
//---------------------------------------------------------------------------

#include "WindowLayer.h"     // iWindowLayer
#include "WindowImpl.h"      // tTJSNI_Window
#include "TouchButton.h"     // krkr::input::TouchButton
#include "VelocityTracker.h" // core/utils

#include <cstdint>
#include <string>
#include <vector>

namespace krkr::sdl3 {

class SdlWindowLayer : public iWindowLayer {
public:
    explicit SdlWindowLayer(tTJSNI_Window *window);
    ~SdlWindowLayer() override;

    SdlWindowLayer(const SdlWindowLayer &) = delete;
    SdlWindowLayer &operator=(const SdlWindowLayer &) = delete;

    [[nodiscard]] tTJSNI_Window *getNativeWindow() const {
        return nativeWindow;
    }

    // --- SDL shell boundary -------------------------------------------------
    // Called by SdlAppDelegate with the raw SDL payload converted to plain
    // types. Coordinates arrive in SDL window units (top-left origin, DPI
    // logical space); the layer maps them into its paint-box space.

    // The SDL window client area changed (logical units; matches the SDL
    // pointer coordinate space). Static: there is exactly one window and
    // every live layer shares the view.
    static void SetSharedViewSize(tjs_int width, tjs_int height);
    [[nodiscard]] static tjs_int GetSharedViewWidth();
    [[nodiscard]] static tjs_int GetSharedViewHeight();

    // Mouse (real or touch-synthesized -- the layer cannot tell them apart,
    // just as the cocos host could not).
    void onPointerMove(float windowX, float windowY);
    void onPointerDown(std::uint32_t sdlButton, float windowX, float windowY);
    void onPointerUp(std::uint32_t sdlButton, float windowX, float windowY);
    void onPointerWheel(float deltaX, float deltaY, float windowX,
                        float windowY);

    // Touch fingers -> TouchButton mouse synthesis
    // (TVPWindow.cpp:155-285 port, see .cpp for behavior notes).
    void onFingerDown(std::uint32_t fingerId, float windowX, float windowY);
    void onFingerMove(std::uint32_t fingerId, float windowX, float windowY);
    void onFingerUp(std::uint32_t fingerId, float windowX, float windowY);

    // Focus-loss path (SdlAppDelegate::cancelInputOnFocusLoss): post a
    // cancellation mouse-up for a held mouse VK, no state change.
    void postSyntheticMouseUp(std::uint32_t key);

    // Reset in-flight touch/bookkeeping state without events, mirroring
    // TVPMainScene::CancelPointerInput(notifyWindow=false).
    void cancelPointerInput();

    // Window chain, same role as TVPWindowLayer's statics.
    static SdlWindowLayer *GetCurrentWindowLayer() {
        return currentWindowLayer;
    }

    // --- iWindowLayer (cpp/core/visual/WindowLayer.h) ----------------------
    void SetPaintBoxSize(tjs_int w, tjs_int h) override;
    bool GetFormEnabled() override;
    void SetDefaultMouseCursor() override;
    void GetCursorPos(tjs_int &x, tjs_int &y) override;
    void SetCursorPos(tjs_int x, tjs_int y) override;
    void SetHintText(const ttstr &text) override;
    void SetAttentionPoint(tjs_int left, tjs_int top,
                           const struct tTVPFont *font) override;
    void ZoomRectangle(tjs_int &left, tjs_int &top, tjs_int &right,
                       tjs_int &bottom) override;
    void BringToFront() override;
    void ShowWindowAsModal() override;
    bool GetVisible() override;
    void SetVisible(bool bVisible) override;
    const char *GetCaption() override;
    void SetCaption(const std::string &) override;
    void SetWidth(tjs_int w) override;
    void SetHeight(tjs_int h) override;
    void SetSize(tjs_int w, tjs_int h) override;
    void GetSize(tjs_int &w, tjs_int &h) override;
    [[nodiscard]] tjs_int GetWidth() const override;
    [[nodiscard]] tjs_int GetHeight() const override;
    void GetWinSize(tjs_int &w, tjs_int &h) override;
    void SetZoom(tjs_int numer, tjs_int denom) override;
    void UpdateDrawBuffer(iTVPTexture2D *tex) override;
    void InvalidateClose() override;
    bool GetWindowActive() override;
    void Close() override;
    // Close-query contract of the old layer (TVPWindow.cpp:905-1013).
    bool OnCloseQuery();
    void OnCloseQueryCalled(bool b) override;
    void InternalKeyDown(tjs_uint16 key, tjs_uint32 shift) override;
    void OnKeyUp(tjs_uint16 vk, int shift) override;
    void OnKeyPress(tjs_uint16 vk, int repeat, bool prevkeystate,
                    bool convertkey) override;
    [[nodiscard]] tTVPImeMode GetDefaultImeMode() const override;
    void SetDefaultImeMode(tTVPImeMode mode, bool reset) override;
    void SetImeMode(tTVPImeMode mode) override;
    void ResetImeMode() override;
    void UpdateWindow(tTVPUpdateType type) override;
    void SetVisibleFromScript(bool b) override;
    void SetUseMouseKey(bool b) override;
    [[nodiscard]] bool GetUseMouseKey() const override;
    void ResetMouseVelocity() override;
    void ResetTouchVelocity(tjs_int id) override;
    bool GetMouseVelocity(float &x, float &y, float &speed) const override;
    void TickBeat() override;

    // TVPWindow.cpp:631-659 port (display toggle between letterbox fit and
    // borderless fill). Pure state; the compositor consumes it (B3).
    void toggleFillScale();
    void RecalcPaintBox();

private:
    // geometry from RecalcPaintBox (TVPWindow.cpp:562-586)
    struct PaintBox {
        float scale = 1.f; // setZoomScale
        float offsetX = 0.f, offsetY = 0.f; // setContentOffset
        float minScale = 1.f, maxScale = 2.f; // setMinScale/maxScale(2x)
    };

    static void SetCurrentWindow(SdlWindowLayer *window);
    void leaveCurrentWindow();

    // Map SDL window coordinates into the paint-box layer space.
    void layerPosition(float windowX, float windowY, tjs_int &x,
                       tjs_int &y) const;
    tjs_uint32 currentShiftState() const;
    tjs_uint32 GetMouseButtonState() const;
    static tTVPMouseButton MouseButtonForKey(std::uint32_t key);
    std::uint32_t touchMoveThresholdSq() const;

    tTJSNI_Window *nativeWindow;

    std::string caption;
    bool Visible = false;
    bool Closing = false, ProgramClosing = false, CanCloseWork = false;
    bool inMode = false; // modal (ShowWindowAsModal)
    int modalResult = 0;

    // contentSize / SetSize target; viewSize is the shared SDL window.
    tjs_int ContentWidth = 0, ContentHeight = 0;
    // SetPaintBoxSize target (the primary layer drawing size).
    tjs_int LayerWidth = 0, LayerHeight = 0;
    // SetZoom (TVPWindow.h:214-229): reduced fraction, cached in
    // ZoomNumer/ZoomDenom plus the reduced "actual" pair.
    tjs_int ActualZoomNumer = 1, ActualZoomDenom = 1;

    PaintBox paintBox;
    // toggleFillScale non-uniform fill factors (B3: compositor reads them).
    float fillScaleX = 1.f, fillScaleY = 1.f;

    // pointer state
    tjs_int LastMouseX = 0, LastMouseY = 0;
    VelocityTracker mouseVelocityTracker;
    VelocityTrackers touchVelocityTracker;

    // touch gesture state (TVPWindow.cpp:155-285 port)
    std::vector<std::uint32_t> touchFingers; // ordered contacts
    krkr::input::TouchButton touchButton;
    bool touchMoved = false;
    float touchStartX = 0.f, touchStartY = 0.f; // _touchPoint
    tjs_uint32 touchBeginTick = 0;

    // options
    bool UseMouseKey = false;
    tTVPImeMode LastSetImeMode = tTVPImeMode::imDisable;
    tTVPImeMode DefaultImeMode = tTVPImeMode::imDisable;
    tjs_int textInputPosY = 0; // SetAttentionPoint feed for the IME area

    // window chain (TVPWindowLayer::_lastWindowLayer/_currentWindowLayer)
    SdlWindowLayer *prevWindow = nullptr;
    SdlWindowLayer *nextWindow = nullptr;
    static SdlWindowLayer *lastWindowLayer;
    static SdlWindowLayer *currentWindowLayer;
    static tjs_int sharedViewWidth, sharedViewHeight;
};

} // namespace krkr::sdl3
