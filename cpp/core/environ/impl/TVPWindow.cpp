#include "TVPWindow.h"
#include "InputState.h"
#include "InputCancellation.h"
#include "InputModifiers.h"
#include "cocos2d/CocosPointerCoordinates.h"
#include "cocos2d/CocosWindowAccess.h"
#include <cocos2d.h>
#include <cocos-ext.h>
#include <stdexcept>

#include "cocos2d/MainScene.h"
#include "Application.h"
#include "DInputMgn.h"
#include "Random.h"
#include "TickCount.h"
#include "VelocityTracker.h"
#include "cocos2d/CCKeyCodeConv.h"
#include "ui/ConsoleWindow.h"
#include "Platform.h"
#include "RenderUtils.h"
#include "RenderManager.h"
#include "SystemImpl.h"
#include "vkdefine.h"

tTJSNI_Window *TVPWindowLayer::TVPGetActiveWindow() {
    if(!_currentWindowLayer)
        return nullptr;
    return _currentWindowLayer->getWindow();
}

TVPWindowLayer::~TVPWindowLayer() {
    if(_lastWindowLayer == this)
        _lastWindowLayer = _prevWindow;
    if(_nextWindow)
        _nextWindow->_prevWindow = _prevWindow;
    if(_prevWindow)
        _prevWindow->_nextWindow = _nextWindow;

    leaveCurrentWindow();
}

void TVPWindowLayer::leaveCurrentWindow() {
    if(_currentWindowLayer != this)
        return;
    // No script callbacks: invalidation can precede the node's destruction.
    TVPMainScene::CancelGameInput(false);
    auto *another = _lastWindowLayer;
    while(another &&
          (another == this || !another->isVisible() ||
           !another->tjsNativeInstance))
        another = another->_prevWindow;
    if(another)
        another->setPosition(0, 0);
    _currentWindowLayer = another;
}

void TVPWindowLayer::InvalidateClose() {
    tjsNativeInstance = nullptr;
    leaveCurrentWindow();
    removeFromParent();
}

bool TVPWindowLayer::init() {
    bool ret = inherit::init();
    setClippingToBounds(false);
    DrawSprite = cocos2d::Sprite::create();
    DrawSprite->setAnchorPoint(cocos2d::Vec2(0, 1)); // top-left
    PrimaryLayerArea = Node::create();
    addChild(PrimaryLayerArea);
    PrimaryLayerArea->addChild(DrawSprite);
    setAnchorPoint(cocos2d::Size::ZERO);
    cocos2d::EventListenerMouse *evmouse =
        cocos2d::EventListenerMouse::create();
    evmouse->onMouseScroll = [this](cocos2d::Event *e) { onMouseScroll(e); };
    evmouse->onMouseDown = [this](cocos2d::Event *e) { onMouseDownEvent(e); };
    evmouse->onMouseUp = [this](cocos2d::Event *e) { onMouseUpEvent(e); };
    evmouse->onMouseMove = [this](cocos2d::Event *e) { onMouseMoveEvent(e); };
    _eventDispatcher->addEventListenerWithSceneGraphPriority(evmouse, this);
    setTouchEnabled(false);
    setVisible(false);
    return ret;
}

void TVPWindowLayer::onMouseDownEvent(cocos2d::Event *_e) {
    if(!TVPMainScene::AcceptsGameInput() || _currentWindowLayer != this)
        return;
    auto *e = dynamic_cast<cocos2d::EventMouse *>(_e);
    switch(e->getMouseButton()) {
        case cocos2d::EventMouse::MouseButton::BUTTON_RIGHT:
            onMouseDown(e->getLocation(), mbRight);
            break;
        case cocos2d::EventMouse::MouseButton::BUTTON_MIDDLE:
            onMouseDown(e->getLocation(), mbMiddle);
            break;
        default:
            break;
    }
}

void TVPWindowLayer::onMouseUpEvent(cocos2d::Event *_e) {
    if(!TVPMainScene::AcceptsGameInput() || _currentWindowLayer != this)
        return;
    auto *e = dynamic_cast<cocos2d::EventMouse *>(_e);
    // Native ups can arrive after focus/window cancellation. Gesture-owned ups
    // are paired by TouchButton instead of testing another device's state.
    switch(e->getMouseButton()) {
        case cocos2d::EventMouse::MouseButton::BUTTON_RIGHT:
            if(krkr::input::sharedState().isDown(VK_RBUTTON))
                onMouseUp(e->getLocation(), mbRight);
            break;
        case cocos2d::EventMouse::MouseButton::BUTTON_MIDDLE:
            if(krkr::input::sharedState().isDown(VK_MBUTTON))
                onMouseUp(e->getLocation(), mbMiddle);
            break;
        default:
            break;
    }
}

void TVPWindowLayer::onMouseMoveEvent(cocos2d::Event *_e) {
    if(!TVPMainScene::AcceptsGameInput())
        return;
    if(!_virutalMouseMode && _currentWindowLayer == this && !_touchMoved) {
        auto *e = dynamic_cast<cocos2d::EventMouse *>(_e);
        cocos2d::Vec2 pt(e->getCursorX(), e->getCursorY());
        onMouseMove(pt);
    }
}

void TVPWindowLayer::onMouseScroll(cocos2d::Event *_e) {
    if(!TVPMainScene::AcceptsGameInput() || _currentWindowLayer != this)
        return;
    auto *e = dynamic_cast<cocos2d::EventMouse *>(_e);
    if(!_windowMgrOverlay) {
        const auto position = krkr::cocosinput::layerPosition(*PrimaryLayerArea,
                                                              e->getLocation());
        tjsNativeInstance->OnMouseWheel(TVPGetCurrentShiftKeyState(),
                                        e->getScrollY() > 0 ? -120 : 120,
                                        position.x, position.y);
        return;
    }
    float scale = getZoomScale();
    if(e->getScrollY() > 0) {
        scale *= 0.9f;
    } else {
        scale *= 1.1f;
    }
    setZoomScale(scale);
    setContentOffset(getContentOffset());
    updateInset();
    relocateContainer(false);
}

bool TVPWindowLayer::onTouchBegan(cocos2d::Touch *touch,
                                  cocos2d::Event *unused_event) {
    if(_windowMgrOverlay)
        return inherit::onTouchBegan(touch, unused_event);
    if(std::find(_touches.begin(), _touches.end(), touch) != _touches.end())
        return true;
    _touches.push_back(touch);
    touchButton.selectForContacts(_touches.size());
    switch(_touches.size()) {
        case 1:
            _touchPoint = touch->getLocation();
            _touchMoved = false;
            _touchLength = 0.0f;
            _touchBeginTick = TVPGetRoughTickCount32();
            break;
        case 2:
            _touchPoint = (_touchPoint + touch->getLocation()) / 2;
            break;
        default:
            break;
    }
    return true;
}

void TVPWindowLayer::onTouchMoved(cocos2d::Touch *touch,
                                  cocos2d::Event *unused_event) {
    if(_windowMgrOverlay)
        return inherit::onTouchMoved(touch, unused_event);
    if(std::find(_touches.begin(), _touches.end(), touch) == _touches.end())
        return;
    if(tjsNativeInstance) {
        if(_touches.size() == 1) {
            if(!_touchMoved &&
               (TVPGetRoughTickCount32() - _touchBeginTick > 150 ||
                _touchPoint.getDistanceSq(touch->getLocation()) >
                    _touchMoveThresholdSq)) {
                const auto position = krkr::cocosinput::layerPosition(
                    *PrimaryLayerArea, _touchPoint);
                _LastMouseX = position.x;
                _LastMouseY = position.y;
                TVPPostInputEvent(new tTVPOnMouseMoveInputEvent(
                    tjsNativeInstance, _LastMouseX, _LastMouseY,
                    TVPGetCurrentShiftKeyState()));
                if(auto button = touchButton.press()) {
                    krkr::input::sharedState().press(*button);
                    TVPPostInputEvent(new tTVPOnMouseDownInputEvent(
                        tjsNativeInstance, _LastMouseX, _LastMouseY,
                        MouseButtonForKey(*button),
                        TVPGetCurrentShiftKeyState()));
                }
                _touchMoved = true;
            } else if(_touchMoved) {
                const auto position = krkr::cocosinput::layerPosition(
                    *PrimaryLayerArea, touch->getLocation());
                _LastMouseX = position.x;
                _LastMouseY = position.y;
                TVPPostInputEvent(
                    new tTVPOnMouseMoveInputEvent(tjsNativeInstance,
                                                  _LastMouseX, _LastMouseY,
                                                  TVPGetCurrentShiftKeyState()),
                    TVP_EPT_DISCARDABLE);
                const std::uint32_t pos =
                    (static_cast<std::uint32_t>(_LastMouseY) << 16) +
                    static_cast<std::uint32_t>(_LastMouseX);
                TVPPushEnvironNoise(&pos, sizeof(pos));
            }
        }
    }
}

void TVPWindowLayer::onTouchEnded(cocos2d::Touch *touch,
                                  cocos2d::Event *unused_event) {
    if(_windowMgrOverlay)
        return inherit::onTouchEnded(touch, unused_event);
    auto touchIter = std::find(_touches.begin(), _touches.end(), touch);
    if(touchIter == _touches.end())
        return; // Ignore a late release/cancel from a discarded gesture.

    const auto button = touchButton.value();
    if(_touches.size() == 1) {
        if(tjsNativeInstance) {
            const auto position = krkr::cocosinput::layerPosition(
                *PrimaryLayerArea, touch->getLocation());
            _LastMouseX = position.x;
            _LastMouseY = position.y;
            if(!_touchMoved) {
                TVPPostInputEvent(new tTVPOnMouseMoveInputEvent(
                    tjsNativeInstance, _LastMouseX, _LastMouseY,
                    TVPGetCurrentShiftKeyState()));
                const auto downPosition = krkr::cocosinput::layerPosition(
                    *PrimaryLayerArea, _touchPoint);
                TVPPostInputEvent(new tTVPOnMouseDownInputEvent(
                    tjsNativeInstance, downPosition.x, downPosition.y,
                    MouseButtonForKey(button), TVPGetCurrentShiftKeyState()));
                TVPPostInputEvent(new tTVPOnClickInputEvent(
                    tjsNativeInstance, _LastMouseX, _LastMouseY));
            }
            krkr::input::sharedState().latchReleasedPress(button);
            TVPPostInputEvent(new tTVPOnMouseUpInputEvent(
                tjsNativeInstance, _LastMouseX, _LastMouseY,
                MouseButtonForKey(button), TVPGetCurrentShiftKeyState()));
        }
    }
    _touches.erase(touchIter);

    if(_touches.size() == 0) {
        _dragging = false;
        _touchMoved = false;
        touchButton.finish();
        krkr::input::sharedState().release(button);
    }
}

void TVPWindowLayer::onTouchCancelled(cocos2d::Touch *touch,
                                      cocos2d::Event *unused_event) {
    if(_windowMgrOverlay)
        return inherit::onTouchCancelled(touch, unused_event);
    auto touchIter = std::find(_touches.begin(), _touches.end(), touch);
    if(touchIter == _touches.end())
        return; // Ignore a late release/cancel from a discarded gesture.
    _touches.erase(touchIter);

    if(_touches.empty()) {
        _dragging = false;
        _touchMoved = false;
        if(auto button = touchButton.finish())
            onMouseUp(touch->getLocation(), MouseButtonForKey(*button));
    }
}

void TVPWindowLayer::onMouseDown(const cocos2d::Vec2 &pt,
                                 tTVPMouseButton button) {
    const auto position =
        krkr::cocosinput::layerPosition(*PrimaryLayerArea, pt);
    _LastMouseX = position.x;
    _LastMouseY = position.y;
    krkr::input::sharedState().press(TVPConvertMouseBtnToVKCode(button));
    TVPPostInputEvent(new tTVPOnMouseDownInputEvent(
        tjsNativeInstance, _LastMouseX, _LastMouseY, button,
        TVPGetCurrentShiftKeyState()));
}

void TVPWindowLayer::onMouseUp(const cocos2d::Vec2 &pt,
                               tTVPMouseButton button) {
    krkr::input::sharedState().release(TVPConvertMouseBtnToVKCode(button));
    const auto position =
        krkr::cocosinput::layerPosition(*PrimaryLayerArea, pt);
    _LastMouseX = position.x;
    _LastMouseY = position.y;
    TVPPostInputEvent(
        new tTVPOnMouseUpInputEvent(tjsNativeInstance, _LastMouseX, _LastMouseY,
                                    button, TVPGetCurrentShiftKeyState()));
}

void TVPWindowLayer::onMouseMove(const cocos2d::Vec2 &pt) {
    const auto position =
        krkr::cocosinput::layerPosition(*PrimaryLayerArea, pt);
    _LastMouseX = position.x;
    _LastMouseY = position.y;
    TVPPostInputEvent(new tTVPOnMouseMoveInputEvent(
                          tjsNativeInstance, _LastMouseX, _LastMouseY,
                          TVPGetCurrentShiftKeyState()),
                      TVP_EPT_DISCARDABLE);
    const std::uint32_t pos = (static_cast<std::uint32_t>(_LastMouseY) << 16) +
        static_cast<std::uint32_t>(_LastMouseX);
    TVPPushEnvironNoise(&pos, sizeof(pos));
}

void TVPWindowLayer::onMouseClick(const cocos2d::Vec2 &pt,
                                  tTVPMouseButton button) {
    const auto position =
        krkr::cocosinput::layerPosition(*PrimaryLayerArea, pt);
    _LastMouseX = position.x;
    _LastMouseY = position.y;
    TVPPostInputEvent(new tTVPOnMouseMoveInputEvent(
                          tjsNativeInstance, _LastMouseX, _LastMouseY,
                          TVPGetCurrentShiftKeyState()),
                      TVP_EPT_DISCARDABLE);
    krkr::input::sharedState().latchReleasedPress(
        TVPConvertMouseBtnToVKCode(button));
    TVPPostInputEvent(new tTVPOnMouseDownInputEvent(
        tjsNativeInstance, _LastMouseX, _LastMouseY, button,
        TVPGetCurrentShiftKeyState()));
    TVPPostInputEvent(
        new tTVPOnClickInputEvent(tjsNativeInstance, _LastMouseX, _LastMouseY));
    TVPPostInputEvent(
        new tTVPOnMouseUpInputEvent(tjsNativeInstance, _LastMouseX, _LastMouseY,
                                    button, TVPGetCurrentShiftKeyState()));
}


void TVPWindowLayer::CancelInput(const std::vector<std::uint32_t> &released) {
    static_assert(VK_RETURN == 0x0d && VK_SPACE == 0x20 && VK_ESCAPE == 0x1b &&
                      VK_PAD1 == 0x1c0 && VK_PAD2 == 0x1c1,
                  "Shared mouse-key bindings must preserve the engine VK ABI");
    auto keys = released;
    if(touchButton.pressed())
        keys.push_back(touchButton.value());
    const auto plan = krkr::input::planInputCancellation(
        keys,
        { UseMouseKey, MouseLeftButtonEmulatedPushed,
          MouseRightButtonEmulatedPushed });
    for(auto *touch : _touches)
        TouchVelocityTracker.end(touch->getID());
    _touches.clear();
    touchButton.reset();
    _dragging = false;
    _touchMoved = false;
    MouseLeftButtonEmulatedPushed = MouseRightButtonEmulatedPushed = false;
    MouseKeyXAccel = MouseKeyYAccel = 0;
    LastMouseMoved = false;
    MouseVelocityTracker.clear();
    if(!tjsNativeInstance)
        return;

    // Do not route through InternalKeyUp: mouse-key release can generate a
    // click there. Direct posts also avoid consuming unread press markers via
    // GetMouseButtonState(), after the focus owner has released all VKs.
    for(auto button : plan.pointerUps) {
        auto mouseButton = mbMiddle;
        if(button == krkr::input::key::leftMouse)
            mouseButton = mbLeft;
        else if(button == krkr::input::key::rightMouse)
            mouseButton = mbRight;
        TVPPostInputEvent(new tTVPOnMouseUpInputEvent(
            tjsNativeInstance, _LastMouseX, _LastMouseY, mouseButton, 0));
    }
    for(auto key : plan.keyUps)
        TVPPostInputEvent(new tTVPOnKeyUpInputEvent(tjsNativeInstance, key, 0));
}

void TVPWindowLayer::TickBeat() {
    if(UseMouseKey && _currentWindowLayer == this &&
       TVPMainScene::AcceptsGameInput())
        GenerateMouseEvent(false, false, false, false);
}

void TVPWindowLayer::SetCursorPos(tjs_int x, tjs_int y) {
    // Cached coordinates are always in the primary layer's top-left space.
    // Presentation coordinates belong to the cursor's parent, never this cache.
    _LastMouseX = x;
    _LastMouseY = y;
    if(_mouseCursor) {
        auto *cursorParent = _mouseCursor->getParent();
        if(!cursorParent)
            cursorParent = getParent();
        if(cursorParent)
            _mouseCursor->setPosition(krkr::cocosinput::cursorPosition(
                *PrimaryLayerArea, *cursorParent, { x, y }));
        _refadeMouseCursor();
    }
}

void TVPWindowLayer::SetImeMode(tTVPImeMode mode) {
    switch(mode) {
        case ::imDisable:
        case ::imClose:
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
            TVPHideIME();
#else
            // #ifdef _MSC_VER
            TVPMainScene::GetInstance()->detachWithIME();
#endif
            break;
        case ::imOpen:
            // TVPMainScene::GetInstance()->attachWithIME();
            // break;
        default:
#if CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID
        {
            cocos2d::Size screenSize = cocos2d::Director::getInstance()
                                           ->getOpenGLView()
                                           ->getFrameSize();
            TVPShowIME(0, _textInputPosY, screenSize.width,
                       screenSize.height / 4);
        }
#else
            // #ifdef _MSC_VER
            TVPMainScene::GetInstance()->attachWithIME();
#endif
        break;
    }
}

void TVPWindowLayer::ZoomRectangle(tjs_int &left, tjs_int &top, tjs_int &right,
                                   tjs_int &bottom) {
    left = static_cast<tjs_int64>(left) * ActualZoomNumer / ActualZoomDenom;
    top = static_cast<tjs_int64>(top) * ActualZoomNumer / ActualZoomDenom;
    right = static_cast<tjs_int64>(right) * ActualZoomNumer / ActualZoomDenom;
    bottom = static_cast<tjs_int64>(bottom) * ActualZoomNumer / ActualZoomDenom;
}

tTVPMouseButton TVPWindowLayer::MouseButtonForKey(std::uint32_t key) {
    switch(key) {
        case krkr::input::key::leftMouse:
            return mbLeft;
        case krkr::input::key::rightMouse:
            return mbRight;
        case krkr::input::key::middleMouse:
            return mbMiddle;
        default:
            throw std::invalid_argument("Unsupported touch mouse button");
    }
}

void TVPWindowLayer::SetCurrentWindow(TVPWindowLayer *window) {
    if(_currentWindowLayer == window || (window && !window->tjsNativeInstance))
        return;
    TVPMainScene::CancelGameInput();
    if(_currentWindowLayer && _currentWindowLayer->tjsNativeInstance)
        _currentWindowLayer->tjsNativeInstance->OnReleaseCapture();
    _currentWindowLayer = window;
}

void TVPWindowLayer::BringToFront() {
    if(!tjsNativeInstance)
        return;
    if(_currentWindowLayer != this) {
        if(_currentWindowLayer) {
            const cocos2d::Size &size = _currentWindowLayer->getViewSize();
            _currentWindowLayer->setPosition(cocos2d::Vec2(size.width, 0));
        }
        SetCurrentWindow(this);
    }
}

void TVPWindowLayer::ShowWindowAsModal() {
    in_mode_ = true;
    setVisible(true);
    BringToFront();
    if(_consoleWin) {
        _consoleWin->removeFromParent();
        _consoleWin = nullptr;
        TVPMainScene::GetInstance()->scheduleUpdate();

        cocos2d::Director::getInstance()->purgeCachedData();
        TVPControlAdDialog(0x10002, 0,
                           0); // ensure to close banner ad
    }
    modal_result_ = 0;
    while(this == _currentWindowLayer && !modal_result_) {
        int remain = TVPDrawSceneOnce(30); // 30 fps
        TVPProcessInputEvents(); // for iOS
        if(::Application->IsTarminate()) {
            modal_result_ = mrCancel;
        } else if(modal_result_ != 0) {
            break;
        } else if(remain > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(remain));
        }
    }
    in_mode_ = false;
}

void TVPWindowLayer::SetVisible(bool bVisible) {
    // OnReleaseCapture checks visibility: revoke capture before hiding, or
    // queued ups and the capture release would both be rejected as inactive.
    if(!bVisible && _currentWindowLayer == this)
        SetCurrentWindow(_prevWindow ? _prevWindow : _nextWindow);
    Visible = bVisible;
    setVisible(bVisible);
    if(bVisible)
        BringToFront();
}

void TVPWindowLayer::ResetDrawSprite() const {
    if(DrawSprite) {
        cocos2d::Size size = getContentSize();
        float scale = static_cast<float>(ActualZoomNumer) / ActualZoomDenom;
        size = size / scale;
        DrawSprite->setScale(_drawTextureScaleX, _drawTextureScaleY);
        DrawSprite->setTextureRect(
            cocos2d::Rect(0, 0, LayerWidth, LayerHeight));
        DrawSprite->setPosition(cocos2d::Vec2(0, size.height));
        PrimaryLayerArea->setContentSize(size);
        PrimaryLayerArea->setScale(scale);
    }
}

void TVPWindowLayer::RecalcPaintBox() {
    if(!LayerWidth || !LayerHeight)
        return;
    ResetDrawSprite();
    cocos2d::Size size = getViewSize();
    cocos2d::Size contSize = getContentSize();
    float r = size.width / size.height;
    float R = contSize.width / contSize.height;
    float scale;
    cocos2d::Vec2 offset;
    if(R > r) {
        scale = size.width / contSize.width;
        offset.x = 0;
        offset.y = (size.height - contSize.height * scale) / 2;
    } else {
        scale = size.height / contSize.height;
        offset.x = (size.width - contSize.width * scale) / 2;
        offset.y = 0;
    }
    setMinScale(scale);
    setMaxScale(scale * 2);
    setZoomScale(scale);
    setContentOffset(offset);
    updateInset();
}

void TVPWindowLayer::SetWidth(tjs_int w) {
    cocos2d::Size size = getContentSize();
    size.width = w;
    setContentSize(size);
    RecalcPaintBox();
}

void TVPWindowLayer::SetHeight(tjs_int h) {
    cocos2d::Size size = getContentSize();
    size.height = h;
    setContentSize(size);
    RecalcPaintBox();
}

void TVPWindowLayer::UpdateDrawBuffer(iTVPTexture2D *tex) {
    if(!tex)
        return;
    cocos2d::Texture2D *tex2d = DrawSprite->getTexture();
    cocos2d::Texture2D *newtex = tex->GetAdapterTexture(tex2d);
    if(tex2d != newtex) {
        DrawSprite->setTexture(newtex);
        float sw, sh;
        tex->GetScale(_drawTextureScaleX, _drawTextureScaleY);
        if(_drawTextureScaleX == 1.f)
            sw = LayerWidth; // tex->GetWidth();
        else {
            sw = tex->GetInternalWidth() *
                (static_cast<float>(LayerWidth) / tex->GetWidth());
            _drawTextureScaleX = 1 / _drawTextureScaleX;
        }
        if(_drawTextureScaleY == 1.f)
            sh = LayerHeight; // tex->GetHeight();
        else {
            sh = tex->GetInternalHeight() *
                (static_cast<float>(LayerHeight) / tex->GetHeight());
            _drawTextureScaleY = 1 / _drawTextureScaleY;
        }
        DrawSprite->setTextureRect(cocos2d::Rect(0, 0, sw, sh));
        DrawSprite->setBlendFunc(cocos2d::BlendFunc::DISABLE);
        ResetDrawSprite();
    }
}

void TVPWindowLayer::toggleFillScale() {
    float scaleX = PrimaryLayerArea->getScaleX();
    float scaleY = PrimaryLayerArea->getScaleY();
    const cocos2d::Size &drawSize = PrimaryLayerArea->getContentSize();
    cocos2d::Size viewSize = getViewSize();
    float R = viewSize.width / viewSize.height;
    float r = drawSize.width / drawSize.height;
    if(fabs(R - r) < 0.01) {
        return; // do not fill border if screen ratio is almost
                // the same
    }

    if(scaleX == scaleY) {
        if(R > r) { // border @ left/right
            _drawSpriteScaleX = R / r;
            PrimaryLayerArea->setScaleX(scaleY * _drawSpriteScaleX);
        } else { // border @ top/bottom
            _drawSpriteScaleY = r / R;
            PrimaryLayerArea->setScaleY(scaleX * _drawSpriteScaleY);
        }
    } else {
        PrimaryLayerArea->setScale(std::min(scaleX, scaleY));
        _drawSpriteScaleX = 1.0f;
        _drawSpriteScaleY = 1.0f;
    }

    updateInset();
    setContentOffset(cocos2d::Vec2::ZERO);
    relocateContainer(false);
}

int TVPWindowLayer::GetMouseButtonState() const {
    int s = 0;
    if(TVPGetAsyncKeyState(VK_LBUTTON))
        s |= ssLeft;
    if(TVPGetAsyncKeyState(VK_RBUTTON))
        s |= ssRight;
    if(TVPGetAsyncKeyState(VK_MBUTTON))
        s |= ssMiddle;
    return s;
}

void TVPWindowLayer::OnMouseDown(tTVPMouseButton button, int shift, int x,
                                 int y) {
    // if (!CanSendPopupHide()) DeliverPopupHide();

    MouseVelocityTracker.addMovement(
        TVPGetRoughTickCount32(), static_cast<float>(x), static_cast<float>(y));

    LastMouseDownX = x;
    LastMouseDownY = y;

    if(tjsNativeInstance) {
        tjs_uint32 s = shift;
        s |= GetMouseButtonState();
        TVPPostInputEvent(
            new tTVPOnMouseDownInputEvent(tjsNativeInstance, x, y, button, s));
    }
}
void TVPWindowLayer::OnMouseClick() const {
    // fire click event
    if(tjsNativeInstance) {
        TVPPostInputEvent(new tTVPOnClickInputEvent(
            tjsNativeInstance, LastMouseDownX, LastMouseDownY));
    }
}

void TVPWindowLayer::GenerateMouseEvent(bool fl, bool fr, bool fu, bool fd) {
    if(!fl && !fr && !fu && !fd) {
        if(TVPGetRoughTickCount32() - 45 < LastMouseKeyTick)
            return;
    }

    bool shift = 0 != (krkr::input::sharedState().query(VK_SHIFT, true));
    bool left = fl || krkr::input::sharedState().query(VK_LEFT, true) ||
        krkr::input::sharedState().query(VK_PADLEFT, true);
    bool right = fr || krkr::input::sharedState().query(VK_RIGHT, true) ||
        krkr::input::sharedState().query(VK_PADRIGHT, true);
    bool up = fu || krkr::input::sharedState().query(VK_UP, true) ||
        krkr::input::sharedState().query(VK_PADUP, true);
    bool down = fd || krkr::input::sharedState().query(VK_DOWN, true) ||
        krkr::input::sharedState().query(VK_PADDOWN, true);

    uint32_t flags = 0;
    if(left || right || up || down)
        flags |= /*MOUSEEVENTF_MOVE*/ 1;

    if(!right && !left && !up && !down) {
        LastMouseMoved = false;
        MouseKeyXAccel = MouseKeyYAccel = 0;
    }

    if(!shift) {
        if(!right && left && MouseKeyXAccel > 0)
            MouseKeyXAccel = -0;
        if(!left && right && MouseKeyXAccel < 0)
            MouseKeyXAccel = 0;
        if(!down && up && MouseKeyYAccel > 0)
            MouseKeyYAccel = -0;
        if(!up && down && MouseKeyYAccel < 0)
            MouseKeyYAccel = 0;
    } else {
        if(left)
            MouseKeyXAccel = -TVP_MOUSE_SHIFT_ACCEL;
        if(right)
            MouseKeyXAccel = TVP_MOUSE_SHIFT_ACCEL;
        if(up)
            MouseKeyYAccel = -TVP_MOUSE_SHIFT_ACCEL;
        if(down)
            MouseKeyYAccel = TVP_MOUSE_SHIFT_ACCEL;
    }

    if(right || left || up || down) {
        if(left)
            if(MouseKeyXAccel > -TVP_MOUSE_MAX_ACCEL)
                MouseKeyXAccel = MouseKeyXAccel ? MouseKeyXAccel - 2 : -2;
        if(right)
            if(MouseKeyXAccel < TVP_MOUSE_MAX_ACCEL)
                MouseKeyXAccel = MouseKeyXAccel ? MouseKeyXAccel + 2 : +2;
        if(!left && !right) {
            if(MouseKeyXAccel > 0)
                MouseKeyXAccel--;
            else if(MouseKeyXAccel < 0)
                MouseKeyXAccel++;
        }

        if(up)
            if(MouseKeyYAccel > -TVP_MOUSE_MAX_ACCEL)
                MouseKeyYAccel = MouseKeyYAccel ? MouseKeyYAccel - 2 : -2;
        if(down)
            if(MouseKeyYAccel < TVP_MOUSE_MAX_ACCEL)
                MouseKeyYAccel = MouseKeyYAccel ? MouseKeyYAccel + 2 : +2;
        if(!up && !down) {
            if(MouseKeyYAccel > 0)
                MouseKeyYAccel--;
            else if(MouseKeyYAccel < 0)
                MouseKeyYAccel++;
        }
    }

    if(flags) {
        _LastMouseX += MouseKeyXAccel >> 1;
        _LastMouseY += MouseKeyYAccel >> 1;
        LastMouseMoved = true;
    }
    LastMouseKeyTick = TVPGetRoughTickCount32();
}

void TVPWindowLayer::InternalKeyDown(tjs_uint16 key, tjs_uint32 shift) {
    tjs_uint32 tick = TVPGetRoughTickCount32();
    TVPPushEnvironNoise(&tick, sizeof(tick));
    TVPPushEnvironNoise(&key, sizeof(key));
    TVPPushEnvironNoise(&shift, sizeof(shift));

    if(UseMouseKey) {
        if(const auto button = krkr::input::mouseKeyButton(key)) {
            cocos2d::Vec2 p(_LastMouseX, _LastMouseY);
            cocos2d::Size size = PrimaryLayerArea->getContentSize();
            if(p.x >= 0 && p.y >= 0 && p.x < size.width && p.y < size.height) {
                if(button == krkr::input::key::leftMouse) {
                    MouseLeftButtonEmulatedPushed = true;
                    OnMouseDown(mbLeft, 0, p.x, p.y);
                }

                if(button == krkr::input::key::rightMouse) {
                    MouseRightButtonEmulatedPushed = true;
                    OnMouseDown(mbRight, 0, p.x, p.y);
                }
            }
            return;
        }

        switch(key) {
            case VK_LEFT:
            case VK_PADLEFT:
                if(MouseKeyXAccel == 0 && MouseKeyYAccel == 0) {
                    GenerateMouseEvent(true, false, false, false);
                    LastMouseKeyTick = TVPGetRoughTickCount32() + 100;
                }
                return;
            case VK_RIGHT:
            case VK_PADRIGHT:
                if(MouseKeyXAccel == 0 && MouseKeyYAccel == 0) {
                    GenerateMouseEvent(false, true, false, false);
                    LastMouseKeyTick = TVPGetRoughTickCount32() + 100;
                }
                return;
            case VK_UP:
            case VK_PADUP:
                if(MouseKeyXAccel == 0 && MouseKeyYAccel == 0) {
                    GenerateMouseEvent(false, false, true, false);
                    LastMouseKeyTick = TVPGetRoughTickCount32() + 100;
                }
                return;
            case VK_DOWN:
            case VK_PADDOWN:
                if(MouseKeyXAccel == 0 && MouseKeyYAccel == 0) {
                    GenerateMouseEvent(false, false, false, true);
                    LastMouseKeyTick = TVPGetRoughTickCount32() + 100;
                }
                return;
            default:;
        }
    }
    TVPPostInputEvent(
        new tTVPOnKeyDownInputEvent(tjsNativeInstance, key, shift));
}

void TVPWindowLayer::InternalKeyUp(tjs_uint16 key, tjs_uint32 shift) {
    tjs_uint32 tick = TVPGetRoughTickCount32();
    TVPPushEnvironNoise(&tick, sizeof(tick));
    TVPPushEnvironNoise(&key, sizeof(key));
    TVPPushEnvironNoise(&shift, sizeof(shift));
    if(tjsNativeInstance) {
        if(UseMouseKey /*&& PaintBox*/) {
            if(const auto button = krkr::input::mouseKeyButton(key)) {
                cocos2d::Vec2 p(_LastMouseX, _LastMouseY);
                cocos2d::Size size = PrimaryLayerArea->getContentSize();
                if(p.x >= 0 && p.y >= 0 && p.x < size.width &&
                   p.y < size.height) {
                    if(button == krkr::input::key::leftMouse) {
                        OnMouseClick();
                        MouseLeftButtonEmulatedPushed = false;
                        OnMouseUp(mbLeft, 0, p.x, p.y);
                    }

                    if(button == krkr::input::key::rightMouse) {
                        MouseRightButtonEmulatedPushed = false;
                        OnMouseUp(mbRight, 0, p.x, p.y);
                    }
                }
                return;
            }
        }

        TVPPostInputEvent(
            new tTVPOnKeyUpInputEvent(tjsNativeInstance, key, shift));
    }
}

void TVPWindowLayer::OnClose(CloseAction &action) {
    if(modal_result_ == 0)
        action = caNone;
    else
        action = caHide;

    if(ProgramClosing) {
        if(tjsNativeInstance) {
            if(tjsNativeInstance->IsMainWindow()) {
                // this is the main window
            } else {
                // not the main window
                action = caFree;
            }
            // if (TVPFullScreenedWindow != this) {
            //  if this is not a fullscreened window
            //	SetVisible(false);
            // }
            iTJSDispatch2 *obj = tjsNativeInstance->GetOwnerNoAddRef();
            tjsNativeInstance->NotifyWindowClose();
            obj->Invalidate(0, nullptr, nullptr, obj);
            tjsNativeInstance = nullptr;
            SetVisible(false);
            scheduleOnce([this](float) { removeFromParent(); }, 0, "remove");
        }
    }
}

bool TVPWindowLayer::OnCloseQuery() {
    // closing actions are 3 patterns;
    // 1. closing action by the user
    // 2. "close" method
    // 3. object invalidation

    if(TVPGetBreathing()) {
        return false;
    }

    // the default event handler will invalidate this object when
    // an onCloseQuery event reaches the handler.
    if(tjsNativeInstance && (modal_result_ == 0 || modal_result_ == mrCancel /* mrCancel=when close button is pushed in modal window */)) {
        if(iTJSDispatch2 *obj = tjsNativeInstance->GetOwnerNoAddRef()) {
            tTJSVariant arg[1] = { true };
            static ttstr eventname(TJS_W("onCloseQuery"));

            if(!ProgramClosing) {
                // close action does not happen immediately
                if(tjsNativeInstance) {
                    TVPPostInputEvent(
                        new tTVPOnCloseInputEvent(tjsNativeInstance));
                }

                Closing = true; // waiting closing...
                //	TVPSystemControl->NotifyCloseClicked();
                return false;
            } else {
                CanCloseWork = true;
                TVPPostEvent(obj, obj, eventname, 0, TVP_EPT_IMMEDIATE, 1, arg);
                TVPDrawSceneOnce(0); // for post event
                // this event happens immediately
                // and does not return until done
                return CanCloseWork; // CanCloseWork is set by the
                                     // event handler
            }
        } else {
            return true;
        }
    } else {
        return true;
    }
}

void TVPWindowLayer::Close() {
    // closing action by "close" method
    if(Closing)
        return; // already waiting closing...

    ProgramClosing = true;
    try {
        // tTVPWindow::Close();
        if(in_mode_) {
            modal_result_ = mrCancel;
        } else if(OnCloseQuery()) {
            CloseAction action = caFree;
            OnClose(action);
            switch(action) {
                case caNone:
                    break;
                case caHide:
                    cocos2d::Hide();
                    break;
                case caMinimize:
                    //::ShowWindow(GetHandle(), SW_MINIMIZE);
                    break;
                case caFree:
                default:
                    scheduleOnce([this](float) { removeFromParent(); }, 0,
                                 "Close");
                    //::PostMessage(GetHandle(),
                    //: TVP_EV_WINDOW_RELEASE, 0, 0);
                    break;
            }
        }
    } catch(...) {
        ProgramClosing = false;
        throw;
    }
    ProgramClosing = false;
}

void TVPWindowLayer::OnCloseQueryCalled(bool b) {
    // closing is allowed by onCloseQuery event handler
    if(!ProgramClosing) {
        // closing action by the user
        if(b) {
            if(in_mode_)
                modal_result_ = 1; // when modal
            else
                SetVisible(false); // just hide

            Closing = false;
            if(tjsNativeInstance) {
                if(tjsNativeInstance->IsMainWindow()) {
                    // this is the main window
                    iTJSDispatch2 *obj = tjsNativeInstance->GetOwnerNoAddRef();
                    obj->Invalidate(0, nullptr, nullptr, obj);
                    // TJSNativeInstance = nullptr; //
                    // ���ζ��A�ǤϼȤ�this����������Ƥ��뤿�ᡢ���Щ`�إ����������ƤϤ����ʤ�
                }
            } else {
                delete this;
            }
        } else {
            Closing = false;
        }
    } else {
        // closing action by the program
        CanCloseWork = b;
    }
}

void TVPWindowLayer::SetUseMouseKey(bool b) {
    UseMouseKey = b;
    if(b) {
        MouseLeftButtonEmulatedPushed = false;
        MouseRightButtonEmulatedPushed = false;
        LastMouseKeyTick = TVPGetRoughTickCount32();
    } else {
        if(MouseLeftButtonEmulatedPushed) {
            MouseLeftButtonEmulatedPushed = false;
            OnMouseUp(mbLeft, 0, _LastMouseX, _LastMouseY);
        }
        if(MouseRightButtonEmulatedPushed) {
            MouseRightButtonEmulatedPushed = false;
            OnMouseUp(mbRight, 0, _LastMouseX, _LastMouseY);
        }
    }
}

void TVPWindowLayer::OnMouseUp(tTVPMouseButton button, int shift, int x,
                               int y) {
    MouseVelocityTracker.addMovement(
        TVPGetRoughTickCount32(), static_cast<float>(x), static_cast<float>(y));
    if(tjsNativeInstance) {
        tjs_uint32 s = shift;
        s |= GetMouseButtonState();
        TVPPostInputEvent(
            new tTVPOnMouseUpInputEvent(tjsNativeInstance, x, y, button, s));
    }
}

void TVPWindowLayer::OnKeyPress(tjs_uint16 vk, int repeat, bool prevkeystate,
                                bool convertkey) {
    if(tjsNativeInstance && vk) {
        if(UseMouseKey && (vk == 0x1b || vk == 13 || vk == 32))
            return;
        TVPPostInputEvent(new tTVPOnKeyPressInputEvent(tjsNativeInstance, vk));
    }
}
cocos2d::Node *TVPGetCocosWindowPrimaryArea(iWindowLayer &window) {
    return dynamic_cast<TVPWindowLayer &>(window).GetPrimaryArea();
}
