#include "SdlAppDelegate.h"

#include "SdlKeyCodeConv.h"
#include "SdlWindowLayer.h"

#include "HostLifecycle.h"      // TVPOnHostForeground/TVPOnHostBackground
#include "WindowIntf.h"         // tTVPOnKeyDown/KeyUp/CloseInputEvent
#include "WindowImpl.h"         // tTJSNI_BaseWindow, TVPGetCurrentShiftKeyState
#include "EventIntf.h"          // TVPPostInputEvent
#include "tvpinputdefs.h"       // TVP_SS_REPEAT, VK_PAD range
#include "InputState.h"         // krkr::input::sharedState()
#include "InputModifiers.h"     // krkr::input::key::*

#include <cstring>

namespace krkr::sdl3 {
namespace {
    // SDL reports touch coordinates normalized to [0,1] regardless of the
    // window scale; rescale into SDL window units for the layer adapter.
    void fingerUnits(SDL_Window *window, float x, float y, float &outX,
                     float &outY) {
        int width = 0, height = 0;
        if(window)
            SDL_GetWindowSize(window, &width, &height);
        outX = x * static_cast<float>(width);
        outY = y * static_cast<float>(height);
    }
} // namespace

SdlAppDelegate::SdlAppDelegate() = default;

SdlAppDelegate::SdlAppDelegate(const Config &config) : config(config) {}

SdlAppDelegate::~SdlAppDelegate() { releaseWindow(); }

bool SdlAppDelegate::init() {
    if(window)
        return true; // already initialized

    // GAMEPAD is included for SDL_EVENT_GAMEPAD_*; the cocos host discovers
    // controllers through its own Controller subsystem (MainScene).
    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD))
        return false;
    ownsSDL = true;

    // Prototype main.cpp:237-241: without this SDL emits QUIT after the last
    // window's close request, which would defeat a script's first onCloseQuery
    // veto. Genuine QUIT (system menu, signal) still arrives as SDL_EVENT_QUIT.
    SDL_SetHintWithPriority(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "0",
                            SDL_HINT_OVERRIDE);

    // GL context attributes, aligned with AppDelegate.cpp:146-148
    // (RGBA8 + depth24 + stencil8).
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    const SDL_WindowFlags flags =
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY |
        (config.fullscreen ? SDL_WINDOW_FULLSCREEN : 0);
    window = SDL_CreateWindow(config.title.c_str(), config.width,
                              config.height, flags);
    if(!window) {
        SDL_Quit();
        ownsSDL = false;
        return false;
    }
    windowID = SDL_GetWindowID(window);

    glContext = SDL_GL_CreateContext(window);
    if(!glContext) {
        releaseWindow();
        return false;
    }
    if(!SDL_GL_MakeCurrent(window, glContext)) {
        // Renderer-less host: MakeCurrent is only fatal if it errors; keep
        // running with unset context, later swap will report failure.
    }
    vsync = SDL_GL_SetSwapInterval(1); // match the 60fps Director pacing

    int width = 0, height = 0;
    SDL_GetWindowSize(window, &width, &height);
    SdlWindowLayer::SetSharedViewSize(width, height);

    // Interactive use starts with the window's real focus state
    // (prototype main.cpp:313-314).
    focused =
        (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    hostForeground = focused;
    wasFullscreen = config.fullscreen;

    openInitialGamepads();
    return true;
}

void SdlAppDelegate::releaseWindow() {
    closeGamepads();
    if(glContext) {
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
    }
    if(window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    if(ownsSDL) {
        SDL_Quit();
        ownsSDL = false;
    }
}

void SdlAppDelegate::run() {
    if(!window)
        return; // init() not called or failed
    running = true;
    while(running) {
        pollEvents();
        if(!running)
            break;
        if(frameCallback)
            frameCallback(); // engine frame: Application->Run() (B2-2 wires it)
        SDL_GL_SwapWindow(window);
        if(!vsync)
            SDL_Delay(16); // prototype main.cpp:448 fallback pacing
    }
}

void SdlAppDelegate::stop() { running = false; }

void SdlAppDelegate::setFullscreen(bool fullscreen) {
    if(!window)
        return;
    if(SDL_SetWindowFullscreen(window, fullscreen))
        wasFullscreen = fullscreen;
}

bool SdlAppDelegate::isFullscreen() const {
    return window &&
        (SDL_GetWindowFlags(window) & SDL_WINDOW_FULLSCREEN) != 0;
}

void SdlAppDelegate::setFrameCallback(FrameCallback callback) {
    frameCallback = std::move(callback);
}

void SdlAppDelegate::setCloseQueryCallback(CloseQueryCallback callback) {
    closeQueryCallback = std::move(callback);
}

void SdlAppDelegate::setContextLostCallback(ContextLostCallback callback) {
    contextLostCallback = std::move(callback);
}

SdlAppDelegate::ContextLostCallback
SdlAppDelegate::getContextLostCallback() const {
    return contextLostCallback;
}

void SdlAppDelegate::setTextInputEnabled(bool enabled) {
    if(!window)
        return;
    if(enabled)
        SDL_StartTextInput(window);
    else
        SDL_StopTextInput(window);
}

void SdlAppDelegate::setIMEDelegate(SdlIMEDelegate *delegate) {
    imeDelegate = delegate;
}

SdlIMEDelegate *SdlAppDelegate::getIMEDelegate() const { return imeDelegate; }

float SdlAppDelegate::getDisplayScale() const {
    if(!window)
        return 1.f;
    const float scale = SDL_GetWindowDisplayScale(window);
    return scale > 0.f ? scale : 1.f;
}

float SdlAppDelegate::getDPI() const {
    // Device::getDPI() replacement. SDL reports a content scale; the legacy
    // baseline is 96dpi. MainScene.cpp:540 (touch threshold: DPI/10) and
    // MainScene.cpp:832 (UI scale factor) consume this via B2-2.
    return 96.f * getDisplayScale();
}

void SdlAppDelegate::getWindowSize(int &width, int &height) const {
    if(window)
        SDL_GetWindowSize(window, &width, &height);
    else {
        width = 0;
        height = 0;
    }
}

void SdlAppDelegate::getWindowSizeInPixels(int &width, int &height) const {
    if(window)
        SDL_GetWindowSizeInPixels(window, &width, &height);
    else {
        width = 0;
        height = 0;
    }
}

void SdlAppDelegate::pollEvents() {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                handleKeyboard(event.key);
                break;

            case SDL_EVENT_TEXT_INPUT:
                // MainScene.cpp:1291 equivalent: committed UTF-8 text.
                if(imeDelegate)
                    imeDelegate->insertText(event.text.text,
                                            std::strlen(event.text.text));
                break;

            case SDL_EVENT_TEXT_EDITING:
                // Preedit: the desktop engine never had this; B2-2 wires the
                // composing-string feedback into the engine.
                if(imeDelegate)
                    imeDelegate->setPreedit(event.edit.text, event.edit.start,
                                            event.edit.length);
                break;

            case SDL_EVENT_MOUSE_MOTION:
                if(event.motion.windowID != windowID)
                    break;
                if(auto *layer = SdlWindowLayer::GetCurrentWindowLayer())
                    layer->onPointerMove(event.motion.x, event.motion.y);
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if(event.button.windowID != windowID || !focused)
                    break;
                if(auto *layer = SdlWindowLayer::GetCurrentWindowLayer())
                    layer->onPointerDown(event.button.button, event.button.x,
                                         event.button.y);
                break;

            case SDL_EVENT_MOUSE_BUTTON_UP:
                if(event.button.windowID != windowID)
                    break;
                if(auto *layer = SdlWindowLayer::GetCurrentWindowLayer())
                    layer->onPointerUp(event.button.button, event.button.x,
                                       event.button.y);
                break;

            case SDL_EVENT_MOUSE_WHEEL: {
                if(event.wheel.windowID != windowID || !focused)
                    break;
                // SdlPointerInput.cpp:76-84: flip direction for "natural"
                // scrolling so sign matches the OS convention.
                const float direction =
                    event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.f
                                                                    : 1.f;
                if(auto *layer = SdlWindowLayer::GetCurrentWindowLayer())
                    layer->onPointerWheel(direction * event.wheel.x,
                                          direction * event.wheel.y,
                                          event.wheel.mouse_x,
                                          event.wheel.mouse_y);
                break;
            }

            case SDL_EVENT_FINGER_DOWN: {
                float x = 0, y = 0;
                fingerUnits(window, event.tfinger.x, event.tfinger.y, x, y);
                if(auto *layer = SdlWindowLayer::GetCurrentWindowLayer())
                    layer->onFingerDown(
                        static_cast<std::uint32_t>(event.tfinger.fingerID), x,
                        y);
                break;
            }

            case SDL_EVENT_FINGER_MOTION: {
                float x = 0, y = 0;
                fingerUnits(window, event.tfinger.x, event.tfinger.y, x, y);
                if(auto *layer = SdlWindowLayer::GetCurrentWindowLayer())
                    layer->onFingerMove(
                        static_cast<std::uint32_t>(event.tfinger.fingerID), x,
                        y);
                break;
            }

            case SDL_EVENT_FINGER_UP: {
                float x = 0, y = 0;
                fingerUnits(window, event.tfinger.x, event.tfinger.y, x, y);
                if(auto *layer = SdlWindowLayer::GetCurrentWindowLayer())
                    layer->onFingerUp(
                        static_cast<std::uint32_t>(event.tfinger.fingerID), x,
                        y);
                break;
            }

            case SDL_EVENT_WINDOW_FOCUS_GAINED:
            case SDL_EVENT_WINDOW_FOCUS_LOST:
            case SDL_EVENT_WINDOW_MINIMIZED:
            case SDL_EVENT_WINDOW_RESTORED:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            case SDL_EVENT_WINDOW_RESIZED:
                handleWindowEvent(event.window);
                break;

            case SDL_EVENT_QUIT:
                if(allowClose())
                    running = false;
                // A vetoed QUIT keeps the loop alive; SDL_HINT_QUIT_ON_LAST_
                // WINDOW_CLOSE=0 ensures SDL does not re-inject it.
                break;

            case SDL_EVENT_GAMEPAD_ADDED:
                if(!gamepads.count(event.gdevice.which)) {
                    if(SDL_Gamepad *pad = SDL_OpenGamepad(event.gdevice.which))
                        gamepads.emplace(event.gdevice.which, pad);
                }
                break;

            case SDL_EVENT_GAMEPAD_REMOVED: {
                auto it = gamepads.find(event.gdevice.which);
                if(it != gamepads.end()) {
                    SDL_CloseGamepad(it->second);
                    gamepads.erase(it);
                }
                break;
            }

            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP: {
                const auto vk = TVPConvertSDLGamepadToVK(
                    static_cast<SDL_GamepadButton>(event.gbutton.button));
                if(!vk)
                    break;
                // Gamepad button events do not auto-repeat in SDL.
                postVirtualKey(vk, event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN,
                               false);
                break;
            }

            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                // TODO(B3): analog axis -> VK hook. The cocos host surface is
                // only the triggers via TVPConvertSDLGamepadAxisToVK
                // (CCKeyCodeConv.cpp:268-271); decide the press/release
                // threshold (and VK_PADLEFT/RIGHT for stick tilt) with the
                // engine input team, then post through postVirtualKey().
                break;

            default:
                break;
        }
    }
}

void SdlAppDelegate::handleWindowEvent(const SDL_WindowEvent &event) {
    if(event.windowID != windowID)
        return;
    switch(event.type) {
        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            if(!focused) {
                focused = true;
                onHostForeground();
            }
            break;

        case SDL_EVENT_WINDOW_FOCUS_LOST:
            if(focused) {
                focused = false;
                // Release held input BEFORE telling the engine it went to the
                // background (InputCancellation semantics).
                cancelInputOnFocusLoss();
                onHostBackground();
            }
            break;

        case SDL_EVENT_WINDOW_MINIMIZED:
            // Windows also loses focus alongside minimize; the focus handlers
            // above own the input release, this only moves the engine state.
            onHostBackground();
            break;

        case SDL_EVENT_WINDOW_RESTORED:
            if(!hostForeground)
                onHostForeground();
            break;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            // The script's onCloseQuery veto: refusing simply keeps the
            // window, matching the twice-to-close behavior of the old host.
            if(allowClose())
                running = false;
            break;

        case SDL_EVENT_WINDOW_RESIZED:
            // Window units (not pixels) match the SDL mouse coordinate space.
            SdlWindowLayer::SetSharedViewSize(event.data1, event.data2);
            // Surface/viewport recreation -> the engine's GL caches are stale.
            notifyContextLost();
            break;

        default:
            break;
    }
}

void SdlAppDelegate::handleKeyboard(const SDL_KeyboardEvent &event) {
    if(event.windowID != windowID)
        return; // foreign window (prototype main.cpp:342-345)
    const auto vk = TVPConvertScancodeToVKCode(event.scancode);
    if(!vk || vk >= 0x200)
        return; // unmapped, or beyond sharedState()'s 0x200-entry table
    if(!focused)
        return; // drop keys while unfocused (prototype main.cpp:352-357)
    postVirtualKey(vk, event.down, event.repeat);
}

void SdlAppDelegate::postVirtualKey(std::uint32_t vk, bool down, bool repeat) {
    tjs_uint32 shift = TVPGetCurrentShiftKeyState();
    if(repeat)
        shift |= TVP_SS_REPEAT;
    auto *win = currentNativeWindow();
    auto &state = krkr::input::sharedState();
    if(down) {
        state.press(vk);
        if(win)
            TVPPostInputEvent(new tTVPOnKeyDownInputEvent(win, vk, shift));
    } else {
        if(!state.release(vk))
            return; // late/duplicate release (prototype main.cpp:362-365)
        if(win)
            TVPPostInputEvent(new tTVPOnKeyUpInputEvent(win, vk, shift));
    }
}

void SdlAppDelegate::onHostForeground() {
    if(hostForeground)
        return;
    hostForeground = true;
    TVPOnHostForeground(); // engine: Application->OnActivate()
}

void SdlAppDelegate::onHostBackground() {
    if(!hostForeground)
        return;
    hostForeground = false;
    TVPOnHostBackground(); // engine: Application->OnDeactivate()
}

void SdlAppDelegate::cancelInputOnFocusLoss() {
    // Hooks the same intent as the InputCancellation plan (the full cocos
    // path is TVPWindowLayer::CancelInput / TVPMainScene::CancelPointerInput):
    // release every held key and deliver matching up events so scripts never
    // see a stuck press. SetUseMouseKey emulation is not ported yet, so
    // planInputCancellation() has nothing to dedupe and the direct loop
    // remains exact; swap it in once mouse-key emulation lands (B2-2).
    auto &state = krkr::input::sharedState();
    const std::vector<std::uint32_t> released = state.releaseAll();
    for(auto key : released) {
        switch(key) {
            case krkr::input::key::leftMouse:
            case krkr::input::key::rightMouse:
            case krkr::input::key::middleMouse:
                if(auto *layer = SdlWindowLayer::GetCurrentWindowLayer())
                    layer->postSyntheticMouseUp(key);
                break;
            default:
                if(auto *win = currentNativeWindow())
                    TVPPostInputEvent(
                        new tTVPOnKeyUpInputEvent(win, key,
                                                  TVPGetCurrentShiftKeyState()));
                break;
        }
    }
    if(auto *layer = SdlWindowLayer::GetCurrentWindowLayer())
        layer->cancelPointerInput(); // drop in-flight touch gestures
}

void SdlAppDelegate::notifyContextLost() {
    dispatchRendererRecreated();
    resetGLExtensionInfoInited();
    if(contextLostCallback)
        contextLostCallback();
}

void SdlAppDelegate::dispatchRendererRecreated() {
    // B3 TODO (engine-side adapter): the cocos host dispatched
    //   cocos2d::Director::getInstance()->getEventDispatcher()
    //       ->dispatchCustomEvent(EVENT_RENDERER_RECREATED);
    // with listeners at RenderManager_ogl.cpp:2968-2980 (render-method
    // rebuild) and YUVSprite.cpp:43-57 (texture re-upload). The SDL shell
    // owns no Director, so the actual dispatch moves into the adapter
    // installed via setContextLostCallback(); keep this body and the string
    // constant documented until that wiring exists.
}

void SdlAppDelegate::resetGLExtensionInfoInited() {
    // B3 TODO (engine-side adapter): TVPGLExtensionInfoInited is a
    // file-static in RenderManager_ogl.cpp:122 guarding idempotent GL
    // extension queries (ogl:123-126). After a context loss the guard must
    // be reset before the next query; the engine must export a reset entry
    // point for the SDL shell to call here.
}

bool SdlAppDelegate::allowClose() const {
    return !closeQueryCallback || closeQueryCallback();
}

tTJSNI_BaseWindow *SdlAppDelegate::currentNativeWindow() const {
    if(auto *layer = SdlWindowLayer::GetCurrentWindowLayer())
        return layer->getNativeWindow();
    return nullptr;
}

void SdlAppDelegate::openInitialGamepads() {
    // Gamepads attached before the window existed never emit ADDED events;
    // SDL_GetGamepads snapshots them (cocos: startDiscoveryController).
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    if(!ids)
        return;
    for(int i = 0; i < count; ++i) {
        if(gamepads.count(ids[i]))
            continue;
        if(SDL_Gamepad *pad = SDL_OpenGamepad(ids[i]))
            gamepads.emplace(ids[i], pad);
    }
    SDL_free(ids);
}

void SdlAppDelegate::closeGamepads() {
    for(auto &entry : gamepads)
        SDL_CloseGamepad(entry.second);
    gamepads.clear();
}

} // namespace krkr::sdl3
