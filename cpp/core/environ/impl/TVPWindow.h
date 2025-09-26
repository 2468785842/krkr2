#pragma once

#include <string>
#include <cocos-ext.h>

#include "tvpinputdefs.h"
#include "WindowImpl.h"
#include "VelocityTracker.h"
#include "ui/ConsoleWindow.h"

class TVPWindowManagerOverlay;

enum {
    ssShift = TVP_SS_SHIFT,
    ssAlt = TVP_SS_ALT,
    ssCtrl = TVP_SS_CTRL,
    ssLeft = TVP_SS_LEFT,
    ssRight = TVP_SS_RIGHT,
    ssMiddle = TVP_SS_MIDDLE,
    ssDouble = TVP_SS_DOUBLE,
    ssRepeat = TVP_SS_REPEAT,
};

enum {
    orientUnknown,
    orientPortrait,
    orientLandscape,
};

namespace cocos2d {
    class Node;
}
static tTVPMouseButton _mouseBtn;

class iWindowLayer {
protected:
    tTVPMouseCursorState MouseCursorState = mcsVisible;
    tjs_int HintDelay = 500;
    tjs_int ZoomDenom = 1; // Zooming factor denominator (setting)
    tjs_int ZoomNumer = 1; // Zooming factor numerator (setting)
    double TouchScaleThreshold = 5, TouchRotateThreshold = 5;

public:
    virtual ~iWindowLayer() = default;
    virtual void SetPaintBoxSize(tjs_int w, tjs_int h) = 0;
    virtual bool GetFormEnabled() = 0;
    virtual void SetDefaultMouseCursor() = 0;
    virtual void GetCursorPos(tjs_int &x, tjs_int &y) = 0;
    virtual void SetCursorPos(tjs_int x, tjs_int y) = 0;
    virtual void SetHintText(const ttstr &text) = 0;
    virtual void SetAttentionPoint(tjs_int left, tjs_int top,
                                   const struct tTVPFont *font) = 0;
    virtual void ZoomRectangle(tjs_int &left, tjs_int &top, tjs_int &right,
                               tjs_int &bottom) = 0;
    virtual void BringToFront() = 0;
    virtual void ShowWindowAsModal() = 0;
    virtual bool GetVisible() = 0;
    virtual void SetVisible(bool bVisible) = 0;
    virtual const char *GetCaption() = 0;
    virtual void SetCaption(const std::string &) = 0;
    virtual void SetWidth(tjs_int w) = 0;
    virtual void SetHeight(tjs_int h) = 0;
    virtual void SetSize(tjs_int w, tjs_int h) = 0;
    virtual void GetSize(tjs_int &w, tjs_int &h) = 0;
    [[nodiscard]] virtual tjs_int GetWidth() const = 0;
    [[nodiscard]] virtual tjs_int GetHeight() const = 0;
    virtual void GetWinSize(tjs_int &w, tjs_int &h) = 0;
    virtual void SetZoom(tjs_int numer, tjs_int denom) = 0;
    virtual void UpdateDrawBuffer(iTVPTexture2D *tex) = 0;
    virtual void InvalidateClose() = 0;
    virtual bool GetWindowActive() = 0;
    virtual void Close() = 0;
    virtual void OnCloseQueryCalled(bool b) = 0;
    virtual void InternalKeyDown(tjs_uint16 key, tjs_uint32 shift) = 0;
    virtual void OnKeyUp(tjs_uint16 vk, int shift) = 0;
    virtual void OnKeyPress(tjs_uint16 vk, int repeat, bool prevkeystate,
                            bool convertkey) = 0;
    [[nodiscard]] virtual tTVPImeMode GetDefaultImeMode() const = 0;
    virtual void SetImeMode(tTVPImeMode mode) = 0;
    virtual void ResetImeMode() = 0;
    virtual void UpdateWindow(tTVPUpdateType type) = 0;
    virtual void SetVisibleFromScript(bool b) = 0;
    virtual void SetUseMouseKey(bool b) = 0;
    [[nodiscard]] virtual bool GetUseMouseKey() const = 0;
    virtual void ResetMouseVelocity() = 0;
    virtual void ResetTouchVelocity(tjs_int id) = 0;
    virtual bool GetMouseVelocity(float &x, float &y, float &speed) const = 0;
    virtual void TickBeat() = 0;
    virtual cocos2d::Node *GetPrimaryArea() = 0;

    void SetZoomNumer(tjs_int n) { SetZoom(n, ZoomDenom); }
    [[nodiscard]] tjs_int GetZoomNumer() const { return ZoomNumer; }
    void SetZoomDenom(tjs_int d) { SetZoom(ZoomNumer, d); }
    [[nodiscard]] tjs_int GetZoomDenom() const { return ZoomDenom; }

    // dummy function
    void RegisterWindowMessageReceiver(tTVPWMRRegMode mode, void *proc,
                                       const void *userdata) {}
    void SetLeft(tjs_int) {}
    void SetTop(tjs_int) {}
    void SetMinWidth(tjs_int) {}
    void SetMaxWidth(tjs_int) {}
    void SetMinHeight(tjs_int) {}
    void SetMaxHeight(tjs_int) {}
    void SetInnerWidth(tjs_int v) { SetWidth(v); }
    void SetInnerHeight(tjs_int v) { SetHeight(v); }
    void SetInnerSize(tjs_int w, tjs_int h) { SetSize(w, h); }
    void SetMinSize(tjs_int, tjs_int) {}
    void SetMaxSize(tjs_int, tjs_int) {}
    void SetPosition(tjs_int, tjs_int) {}
    void SetBorderStyle(tTVPBorderStyle) {}
    void SetStayOnTop(bool) {}
    void SetFullScreenMode(bool) {}
    tjs_int GetLeft() const { return 0; }
    tjs_int GetTop() const { return 0; }
    tjs_int GetMinWidth() const { return 0; }
    tjs_int GetMaxWidth() const { return 1920; }
    tjs_int GetMinHeight() const { return 0; }
    tjs_int GetMaxHeight() const { return 1080; }
    tjs_int GetInnerWidth() const { return GetWidth(); }
    tjs_int GetInnerHeight() const { return GetHeight(); }
    bool GetStayOnTop() const { return false; }
    bool GetFullScreenMode() const { return false; }
    [[nodiscard]] tTVPBorderStyle GetBorderStyle() const { return bsNone; }
    void SetTrapKey(bool b) {}
    [[nodiscard]] bool GetTrapKey() const { return false; }
    void RemoveMaskRegion() {}
    void SetMouseCursorState(tTVPMouseCursorState mcs) {
        MouseCursorState = mcs;
    }
    [[nodiscard]] tTVPMouseCursorState GetMouseCursorState() const {
        return MouseCursorState;
    }
    void HideMouseCursor() {}
    void SetFocusable(bool b) {}
    [[nodiscard]] bool GetFocusable() const { return true; }
    int GetDisplayRotate() { return 0; }
    int GetDisplayOrientation() { return orientLandscape; }
    void SetEnableTouch(bool b) {}
    [[nodiscard]] bool GetEnableTouch() const { return false; }
    void SetHintDelay(tjs_int delay) { HintDelay = delay; }
    [[nodiscard]] tjs_int GetHintDelay() const { return HintDelay; }
    void SetInnerSunken(bool b) {}
    [[nodiscard]] bool GetInnerSunken() const { return false; }

    // TODO
    void SetMouseCursor(tjs_int handle) {}
    void SetHintText(iTJSDispatch2 *sender, const ttstr &text) {}
    void DisableAttentionPoint() {}
    static void GetVideoOffset(tjs_int &ofsx, tjs_int &ofsy) {
        ofsx = 0;
        ofsy = 0;
    }
    void SetTouchScaleThreshold(double threshold) {
        TouchScaleThreshold = threshold;
    }
    double GetTouchScaleThreshold() const { return TouchScaleThreshold; }
    void SetTouchRotateThreshold(double threshold) {
        TouchRotateThreshold = threshold;
    }
    double GetTouchRotateThreshold() const { return TouchRotateThreshold; }
    [[nodiscard]] tjs_real GetTouchPointStartX(tjs_int index) const {
        return 0;
    }
    [[nodiscard]] tjs_real GetTouchPointStartY(tjs_int index) const {
        return 0;
    }
    [[nodiscard]] tjs_real GetTouchPointX(tjs_int index) const { return 0; }
    [[nodiscard]] tjs_real GetTouchPointY(tjs_int index) const { return 0; }
    [[nodiscard]] tjs_int GetTouchPointID(tjs_int index) const { return 0; }
    [[nodiscard]] tjs_int GetTouchPointCount() const { return 0; }
    bool GetTouchVelocity(tjs_int id, float &x, float &y, float &speed) const {
        return false;
    }
    void ResetDrawDevice() {}
    void SendCloseMessage() {}
    void BeginMove() {}
    void SetLayerLeft(tjs_int l) {}
    [[nodiscard]] tjs_int GetLayerLeft() const { return 0; }
    void SetLayerTop(tjs_int t) {}
    [[nodiscard]] tjs_int GetLayerTop() const { return 0; }
    void SetLayerPosition(tjs_int l, tjs_int t) {}
    void SetShowScrollBars(bool b) {}
    [[nodiscard]] bool GetShowScrollBars() const { return true; }
};

class TVPWindowLayer : public cocos2d::extension::ScrollView,
                       public iWindowLayer {
public:
    static inline TVPWindowManagerOverlay *_windowMgrOverlay = nullptr;
    static inline TVPWindowLayer *_lastWindowLayer, *_currentWindowLayer;
    static inline TVPConsoleWindow *_consoleWin = nullptr;

    static inline float _touchMoveThresholdSq;
    static inline cocos2d::Node *_mouseCursor;
    static inline int _touchBeginTick;
    static inline bool _virutalMouseMode = false;
    static inline tjs_uint8 _scancode[0x200];

    bool Closing = false, ProgramClosing = false, CanCloseWork = false;
    bool in_mode_ = false; // is modal
    int modal_result_ = 0;
    enum CloseAction { caNone, caHide, caFree, caMinimize };

    static bool TVPGetJoyPadAsyncState(tjs_uint keycode, bool getcurrent) {
        if(keycode >= std::size(_scancode))
            return false;
        tjs_uint8 code = _scancode[keycode];
        _scancode[keycode] &= 1;
        return code & (getcurrent ? 1 : 0x10);
    }

    static bool TVPGetKeyMouseAsyncState(tjs_uint keycode, bool getcurrent) {
        if(keycode >= std::size(_scancode))
            return false;
        tjs_uint8 code = _scancode[keycode];
        _scancode[keycode] &= 1;
        return code & (getcurrent ? 1 : 0x10);
    }

    static void _refadeMouseCursor() {
        _mouseCursor->stopAllActions();
        _mouseCursor->setOpacity(255);
        _mouseCursor->runAction(cocos2d::Sequence::createWithTwoActions(
            cocos2d::DelayTime::create(3), cocos2d::FadeOut::create(0.3f)));
    }

    static tTJSNI_Window *TVPGetActiveWindow();

    static constexpr int TVP_MOUSE_MAX_ACCEL = 30;
    static constexpr int TVP_MOUSE_SHIFT_ACCEL = 40;
    static constexpr int TVP_TOOLTIP_SHOW_DELAY = 500;

private:
    typedef cocos2d::extension::ScrollView inherit;
    tTJSNI_Window *tjsNativeInstance;
    tjs_int ActualZoomDenom; // Zooming factor denominator (actual)
    tjs_int ActualZoomNumer; // Zooming factor numerator (actual)
    cocos2d::Sprite *DrawSprite = nullptr;
    Node *PrimaryLayerArea = nullptr;
    int LayerWidth = 0, LayerHeight = 0;
    // iTVPTexture2D *DrawTexture = nullptr;
    TVPWindowLayer *_prevWindow, *_nextWindow;

    friend class TVPWindowManagerOverlay;

    friend class TVPMainScene;

    int _LastMouseX = 0, _LastMouseY = 0;
    std::string _caption;
    //	std::map<tTJSNI_BaseVideoOverlay*, Sprite*> _AllOverlay;
    float _drawSpriteScaleX = 1.0f, _drawSpriteScaleY = 1.0f;
    float _drawTextureScaleX = 1.f, _drawTextureScaleY = 1.f;
    bool UseMouseKey = false, MouseLeftButtonEmulatedPushed = false,
         MouseRightButtonEmulatedPushed = false;
    bool LastMouseMoved = false, Visible = false;
    tjs_uint32 LastMouseKeyTick = 0;
    tjs_int MouseKeyXAccel = 0;
    tjs_int MouseKeyYAccel = 0;
    int LastMouseDownX = 0, LastMouseDownY = 0;
    VelocityTrackers TouchVelocityTracker;
    VelocityTracker MouseVelocityTracker;
    tTVPImeMode LastSetImeMode = tTVPImeMode::imDisable;
    tTVPImeMode DefaultImeMode = tTVPImeMode::imDisable;

public:
    explicit TVPWindowLayer(tTJSNI_Window *w) : tjsNativeInstance(w) {
        _nextWindow = nullptr;
        _prevWindow = _lastWindowLayer;
        _lastWindowLayer = this;
        ActualZoomDenom = 1;
        ActualZoomNumer = 1;
        if(_prevWindow) {
            _prevWindow->_nextWindow = this;
        }
    }

    ~TVPWindowLayer() override;

    bool init() override;

    static TVPWindowLayer *create(tTJSNI_Window *w) {
        auto *ret = new TVPWindowLayer(w);
        ret->init();
        ret->autorelease();
        return ret;
    }

    cocos2d::Node *GetPrimaryArea() override { return PrimaryLayerArea; }

    void onMouseDownEvent(cocos2d::Event *_e);

    void onMouseUpEvent(cocos2d::Event *_e);

    void onMouseMoveEvent(cocos2d::Event *_e);

    void onMouseScroll(cocos2d::Event *_e);

    bool onTouchBegan(cocos2d::Touch *touch,
                      cocos2d::Event *unused_event) override;

    void onTouchMoved(cocos2d::Touch *touch,
                      cocos2d::Event *unused_event) override;

    void onTouchEnded(cocos2d::Touch *touch,
                      cocos2d::Event *unused_event) override;

    void onTouchCancelled(cocos2d::Touch *touch,
                          cocos2d::Event *unused_event) override;

    void onMouseDown(const cocos2d::Vec2 &pt);

    void onMouseUp(const cocos2d::Vec2 &pt);

    void onMouseMove(const cocos2d::Vec2 &pt);

    void onMouseClick(const cocos2d::Vec2 &pt);

    tTJSNI_Window *getWindow() { return tjsNativeInstance; }

    void SetPaintBoxSize(tjs_int w, tjs_int h) override {
        LayerWidth = w;
        LayerHeight = h;
        RecalcPaintBox();
    }

    bool GetFormEnabled() override { return isVisible(); }

    void SetDefaultMouseCursor() override {}

    void GetCursorPos(tjs_int &x, tjs_int &y) override {
        x = _LastMouseX;
        y = _LastMouseY;
    }

    void SetCursorPos(tjs_int x, tjs_int y) override;

    void SetHintText(const ttstr &text) override {}

    tjs_int _textInputPosY{};

    void SetAttentionPoint(tjs_int left, tjs_int top,
                           const struct tTVPFont *font) override {
        _textInputPosY = top;
    }

    void SetImeMode(tTVPImeMode mode) override;

    void ZoomRectangle(tjs_int &left, tjs_int &top, tjs_int &right,
                       tjs_int &bottom) override;

    void BringToFront() override;

    void ShowWindowAsModal() override;

    bool GetVisible() override { return isVisible(); }

    void SetVisible(bool bVisible) override;

    const char *GetCaption() override { return _caption.c_str(); }

    void SetCaption(const std::string &s) override { _caption = s; }

    void ResetDrawSprite() const;

    void RecalcPaintBox();

    void SetWidth(tjs_int w) override;

    void SetHeight(tjs_int h) override;

    void SetSize(tjs_int w, tjs_int h) override {
        setContentSize(cocos2d::Size(w, h));
        RecalcPaintBox();
    }

    void GetSize(tjs_int &w, tjs_int &h) override {
        cocos2d::Size size = getContentSize();
        w = static_cast<tjs_int>(size.width);
        h = static_cast<tjs_int>(size.height);
    }

    void GetWinSize(tjs_int &w, tjs_int &h) override {
        cocos2d::Size size = getViewSize();
        w = static_cast<tjs_int>(size.width);
        h = static_cast<tjs_int>(size.height);
    }

    tjs_int GetWidth() const override { return getContentSize().width; }

    tjs_int GetHeight() const override { return getContentSize().height; }

    void SetZoom(tjs_int numer, tjs_int denom) override {
        tjs_int a = numer;
        tjs_int b = denom;
        while(b) {
            tjs_int t = b;
            b = a % b;
            a = t;
        }
        numer = numer / a;
        denom = denom / a;
        ZoomNumer = numer;
        ZoomDenom = denom;
        ActualZoomDenom = denom;
        ActualZoomNumer = numer;
        RecalcPaintBox();
    }

    void UpdateDrawBuffer(iTVPTexture2D *tex) override;

    void toggleFillScale();

    void InvalidateClose() override {
        // closing action by object invalidation;
        // this will not cause any user confirmation of closing the
        // window.

        // TVPRemoveWindowLayer(this);
        this->removeFromParent(); // and delete this
    }

    bool GetWindowActive() override { return _currentWindowLayer == this; }

    int GetMouseButtonState() const;

    void OnMouseDown(tTVPMouseButton button, int shift, int x, int y);

    void OnMouseClick() const;

    void GenerateMouseEvent(bool fl, bool fr, bool fu, bool fd);

    void InternalKeyDown(tjs_uint16 key, tjs_uint32 shift) override;

    void InternalKeyUp(tjs_uint16 key, tjs_uint32 shift);

    void OnKeyUp(tjs_uint16 vk, int shift) override {
        tjs_uint32 s = shift;
        s |= GetMouseButtonState();
        InternalKeyUp(vk, s);
    }

    void OnKeyPress(tjs_uint16 vk, int repeat, bool prevkeystate,
                    bool convertkey) override;

    tTVPImeMode GetDefaultImeMode() const override { return DefaultImeMode; }

    void ResetImeMode() override { SetImeMode(DefaultImeMode); }

    void OnClose(CloseAction &action);

    bool OnCloseQuery();

    void Close() override;

    void OnCloseQueryCalled(bool b) override;

    void UpdateWindow(tTVPUpdateType type) override {
        if(tjsNativeInstance) {
            tTVPRect r;
            r.left = 0;
            r.top = 0;
            r.right = LayerWidth;
            r.bottom = LayerHeight;
            tjsNativeInstance->NotifyWindowExposureToLayer(r);
            TVPDeliverWindowUpdateEvents();
        }
    }

    void SetVisibleFromScript(bool b) override { SetVisible(b); }

    void SetUseMouseKey(bool b) override;

    bool GetUseMouseKey() const override { return UseMouseKey; }

    void OnMouseUp(tTVPMouseButton button, int shift, int x, int y);

    void ResetTouchVelocity(tjs_int id) override {
        TouchVelocityTracker.end(id);
    }

    void ResetMouseVelocity() override { MouseVelocityTracker.clear(); }

    bool GetMouseVelocity(float &x, float &y, float &speed) const override {
        if(MouseVelocityTracker.getVelocity(x, y)) {
            speed = hypotf(x, y);
            return true;
        }
        return false;
    }

    void TickBeat() override {
        bool focused = _currentWindowLayer == this;
        // mouse key
        if(UseMouseKey && focused) {
            GenerateMouseEvent(false, false, false, false);
        }
    }
};
