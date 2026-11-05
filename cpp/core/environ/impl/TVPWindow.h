#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <cocos-ext.h>

#include "WindowLayer.h"
#include "TouchButton.h"
#include "WindowImpl.h"
#include "VelocityTracker.h"
#include "ui/ConsoleWindow.h"

class TVPWindowManagerOverlay;

class TVPWindowLayer : public cocos2d::extension::ScrollView,
                       public iWindowLayer {
public:
    static inline TVPWindowManagerOverlay *_windowMgrOverlay = nullptr;
    static inline TVPWindowLayer *_lastWindowLayer, *_currentWindowLayer;
    static inline TVPConsoleWindow *_consoleWin = nullptr;

    static inline float _touchMoveThresholdSq;
    static inline cocos2d::Node *_mouseCursor;
    static inline bool _virutalMouseMode = false;

    bool Closing = false, ProgramClosing = false, CanCloseWork = false;
    bool in_mode_ = false; // is modal
    int modal_result_ = 0;
    enum CloseAction { caNone, caHide, caFree, caMinimize };

    static void _refadeMouseCursor() {
        _mouseCursor->stopAllActions();
        _mouseCursor->setOpacity(255);
        _mouseCursor->runAction(cocos2d::Sequence::createWithTwoActions(
            cocos2d::DelayTime::create(3), cocos2d::FadeOut::create(0.3f)));
    }

    static tTJSNI_Window *TVPGetActiveWindow();
    static void SetCurrentWindow(TVPWindowLayer *window);

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
    int _touchBeginTick = 0;
    krkr::input::TouchButton touchButton;
    static tTVPMouseButton MouseButtonForKey(std::uint32_t key);
    void leaveCurrentWindow();
    static void UpdateImeMode();
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

    cocos2d::Node *GetPrimaryArea() { return PrimaryLayerArea; }

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

    void onMouseDown(const cocos2d::Vec2 &pt, tTVPMouseButton button);

    void onMouseUp(const cocos2d::Vec2 &pt, tTVPMouseButton button);

    void onMouseMove(const cocos2d::Vec2 &pt);

    void onMouseClick(const cocos2d::Vec2 &pt, tTVPMouseButton button);

    // The caller released the relevant VKs; cancel gestures and enqueue ups
    // only.
    void CancelInput(const std::vector<std::uint32_t> &released);

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

    // Revoke input before a retained/autoreleased node can outlive its native
    // instance.
    void InvalidateClose() override;

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

    void SetDefaultImeMode(tTVPImeMode mode, bool reset) override {
        DefaultImeMode = mode;
        if(reset)
            ResetImeMode();
    }

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

    void TickBeat() override;
};
