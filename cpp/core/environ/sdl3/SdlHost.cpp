#include "SdlHost.h"

#include <SDL3/SDL.h>

#include "tjsCommHead.h"
#include "Application.h"
#include "ConfigManager/GlobalConfigManager.h"
#include "ConfigManager/IndividualConfigManager.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "ConfigManager/PreferenceDefaults.h"
#include "DialogBackend.h"
#include "EventIntf.h"
#include "HostFrame.h"
#include "HostLifecycle.h"
#include "InputModifiers.h"
#include "InputState.h"
#include "MsgIntf.h"
#include "PointerCoordinates.h"
#include "RenderManager.h"
#include "SysInitIntf.h"
#include "TextInput.h"
#include "TickCount.h"
#include "TVPLog.h"
#include "VelocityTracker.h"
#include "WindowImpl.h"
#include "WindowLayer.h"
#include "tvpfontstruc.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

extern std::thread::id TVPMainThreadID;
std::string TVPGetCurrentLanguage();

namespace krkr::sdl {
    namespace {
        void check(bool success) {
            if(!success)
                throw std::runtime_error(std::string("SDL3: ") +
                                         SDL_GetError());
        }

        bool UsesWayland() {
            const auto *driver = SDL_GetCurrentVideoDriver();
            return driver && std::strcmp(driver, "wayland") == 0;
        }

        struct VideoSubsystem {
            VideoSubsystem() {
                // A native window close is resolved by the script query.
                check(
                    SDL_SetHintWithPriority(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE,
                                            "0", SDL_HINT_OVERRIDE));
                check(SDL_InitSubSystem(SDL_INIT_VIDEO));
                // X11 can use a native software surface. Its implicit GPU
                // framebuffer produces black native capture under Xvfb.
                if(!UsesWayland())
                    check(SDL_SetHintWithPriority(
                        SDL_HINT_FRAMEBUFFER_ACCELERATION, "0",
                        SDL_HINT_OVERRIDE));
            }
            ~VideoSubsystem() { SDL_QuitSubSystem(SDL_INIT_VIDEO); }
        };

        unsigned virtualKey(const SDL_KeyboardEvent &event) {
            const auto key =
                SDL_GetKeyFromScancode(event.scancode, SDL_KMOD_NONE, true);
            if(key >= SDLK_A && key <= SDLK_Z)
                return 'A' + (key - SDLK_A);
            if(key >= SDLK_0 && key <= SDLK_9)
                return '0' + (key - SDLK_0);
            if(key >= SDLK_F1 && key <= SDLK_F12)
                return 0x70 + key - SDLK_F1;
            if(key >= SDLK_F13 && key <= SDLK_F24)
                return 0x7c + key - SDLK_F13;
            if(key >= SDLK_KP_1 && key <= SDLK_KP_9)
                return 0x61 + key - SDLK_KP_1;
            switch(key) {
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    return input::key::shift;
                case SDLK_LCTRL:
                case SDLK_RCTRL:
                    return input::key::control;
                case SDLK_LALT:
                case SDLK_RALT:
                    return input::key::alt;
                case SDLK_BACKSPACE:
                    return 0x08;
                case SDLK_TAB:
                    return 0x09;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    return 0x0d;
                case SDLK_ESCAPE:
                    return 0x1b;
                case SDLK_SPACE:
                    return 0x20;
                case SDLK_LEFT:
                    return 0x25;
                case SDLK_UP:
                    return 0x26;
                case SDLK_RIGHT:
                    return 0x27;
                case SDLK_DOWN:
                    return 0x28;
                case SDLK_PAUSE:
                    return 0x13;
                case SDLK_CAPSLOCK:
                    return 0x14;
                case SDLK_PAGEUP:
                    return 0x21;
                case SDLK_PAGEDOWN:
                    return 0x22;
                case SDLK_END:
                    return 0x23;
                case SDLK_HOME:
                    return 0x24;
                case SDLK_PRINTSCREEN:
                    return 0x2c;
                case SDLK_INSERT:
                    return 0x2d;
                case SDLK_DELETE:
                    return 0x2e;
                case SDLK_LGUI:
                    return 0x5b;
                case SDLK_RGUI:
                    return 0x5c;
                case SDLK_APPLICATION:
                case SDLK_MENU:
                    return 0x5d;
                case SDLK_KP_0:
                    return 0x60;
                case SDLK_KP_MULTIPLY:
                    return 0x6a;
                case SDLK_KP_PLUS:
                    return 0x6b;
                case SDLK_KP_MINUS:
                    return 0x6d;
                case SDLK_KP_PERIOD:
                    return 0x6e;
                case SDLK_KP_DIVIDE:
                    return 0x6f;
                case SDLK_NUMLOCKCLEAR:
                    return 0x90;
                case SDLK_SCROLLLOCK:
                    return 0x91;
                case SDLK_SEMICOLON:
                    return 0xba;
                case SDLK_EQUALS:
                    return 0xbb;
                case SDLK_COMMA:
                    return 0xbc;
                case SDLK_MINUS:
                    return 0xbd;
                case SDLK_PERIOD:
                    return 0xbe;
                case SDLK_SLASH:
                    return 0xbf;
                case SDLK_GRAVE:
                    return 0xc0;
                case SDLK_LEFTBRACKET:
                    return 0xdb;
                case SDLK_BACKSLASH:
                    return 0xdc;
                case SDLK_RIGHTBRACKET:
                    return 0xdd;
                case SDLK_APOSTROPHE:
                    return 0xde;
                default:
                    return 0;
            }
        }

        constexpr std::array<unsigned, 5> buttonKeys{ input::key::leftMouse,
                                                      input::key::rightMouse,
                                                      input::key::middleMouse,
                                                      0x05, 0x06 };

        std::optional<tTVPMouseButton> mouseButton(unsigned key) {
            const auto it =
                std::find(buttonKeys.begin(), buttonKeys.end(), key);
            if(it == buttonKeys.end())
                return std::nullopt;
            return static_cast<tTVPMouseButton>(it - buttonKeys.begin());
        }

        class Window;
        void PumpFrame();
        Window *focusedWindow = nullptr;
        Window *modalWindow = nullptr;

        class Window final : public iWindowLayer,
                             public std::enable_shared_from_this<Window> {
            using WindowPtr =
                std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>;
            using RendererPtr =
                std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)>;
            using TexturePtr =
                std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)>;

            tTJSNI_Window *native;
            WindowPtr window{ nullptr, SDL_DestroyWindow };
            RendererPtr renderer{ nullptr, SDL_DestroyRenderer };
            TexturePtr texture{ nullptr, SDL_DestroyTexture };
            std::string caption = "krkr2";
            int width = 640, height = 480;
            int layerWidth = 640, layerHeight = 480;
            unsigned textureWidth = 0, textureHeight = 0;
            int mouseX = 0, mouseY = 0;
            bool visible = false, focused = false, minimized = false;
            bool pointerInside = false;
            bool dirty = true, closePending = false;
            bool programClosing = false;
            std::array<tjs_uint16, SDL_SCANCODE_COUNT> heldKeys{};
            std::optional<SDL_FPoint> leftPress;
            float wheelRemainder = 0;
            tTVPImeMode defaultIme = imDisable, imeMode = imDisable;
            std::optional<SDL_Rect> attention;
            VelocityTracker mouseVelocity;
            tTVPBorderStyle borderStyle = bsSizeable;

        public:
            explicit Window(tTJSNI_Window *owner) : native(owner) {
                window.reset(
                    SDL_CreateWindow(caption.c_str(), width, height,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN |
                                         SDL_WINDOW_HIGH_PIXEL_DENSITY));
                check(window != nullptr);
                // Wayland has no SDL native window-surface framebuffer.
                // Only presentation uses EGL; the engine still uploads RGBA.
                renderer.reset(SDL_CreateRenderer(
                    window.get(),
                    UsesWayland() ? "opengles2,opengl" : "software"));
                check(renderer != nullptr);
                SetPaintBoxSize(layerWidth, layerHeight);
                G_CoreLog.info("SDL3 video={}, renderer={}",
                               SDL_GetCurrentVideoDriver(),
                               SDL_GetRendererName(renderer.get()));
            }

            bool Live() const { return native != nullptr; }
            bool Main() const { return Live() && native->IsMainWindow(); }
            bool Owns(SDL_Window *candidate) const {
                return candidate && candidate == window.get();
            }
            void SetPaintBoxSize(tjs_int w, tjs_int h) override {
                if(w <= 0 || h <= 0)
                    return;
                check(SDL_SetRenderLogicalPresentation(
                    renderer.get(), w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX));
                layerWidth = w;
                layerHeight = h;
                UpdateAttentionPoint();
                dirty = true;
            }
            bool GetFormEnabled() override {
                return Live() && visible &&
                    (!modalWindow || modalWindow == this);
            }
            void SetDefaultMouseCursor() override {
                SDL_SetCursor(SDL_GetDefaultCursor());
            }
            void GetCursorPos(tjs_int &x, tjs_int &y) override {
                x = mouseX;
                y = mouseY;
            }
            void SetCursorPos(tjs_int x, tjs_int y) override {
                float windowX, windowY;
                check(SDL_RenderCoordinatesToWindow(renderer.get(), x, y,
                                                    &windowX, &windowY));
                SDL_WarpMouseInWindow(window.get(), windowX, windowY);
                mouseX = x;
                mouseY = y;
            }
            void SetHintText(const ttstr &) override {}
            void SetAttentionPoint(tjs_int x, tjs_int y,
                                   const tTVPFont *font) override {
                attention =
                    SDL_Rect{ x, y, 1, font ? std::max(1, font->Height) : 1 };
                UpdateAttentionPoint();
            }
            void DisableAttentionPoint() override {
                attention.reset();
                UpdateAttentionPoint();
            }
            void ZoomRectangle(tjs_int &left, tjs_int &top, tjs_int &right,
                               tjs_int &bottom) override {
                const auto first = WindowPoint(left, top);
                const auto last = WindowPoint(right, bottom);
                left = first.x;
                top = first.y;
                right = last.x;
                bottom = last.y;
            }
            void BringToFront() override {
                check(SDL_RaiseWindow(window.get()));
            }
            void ShowWindowAsModal() override {
                if(modalWindow == this)
                    TVPThrowExceptionMessage(TJS_W("Window is already modal"));
                auto lifetime = shared_from_this();
                auto previousFocus = focusedWindow && focusedWindow != this
                    ? focusedWindow->shared_from_this()
                    : nullptr;
                if(previousFocus)
                    previousFocus->SetFocused(false);
                auto *previousModal = std::exchange(modalWindow, this);
                try {
                    SetVisible(true);
                    BringToFront();
                    while(Live() && GetVisible() && !Application->IsTarminate())
                        PumpFrame();
                } catch(...) {
                    modalWindow = previousModal;
                    throw;
                }
                modalWindow = previousModal;
                if(previousFocus && previousFocus->GetFormEnabled()) {
                    previousFocus->BringToFront();
                    previousFocus->SetFocused(
                        previousFocus->Owns(SDL_GetKeyboardFocus()));
                }
            }
            bool GetVisible() override { return visible; }
            void SetVisible(bool value) override {
                if(visible == value)
                    return;
                check(value ? SDL_ShowWindow(window.get())
                            : SDL_HideWindow(window.get()));
                UpdateVisibility(value);
            }
            const char *GetCaption() override { return caption.c_str(); }
            void SetCaption(const std::string &value) override {
                check(SDL_SetWindowTitle(window.get(), value.c_str()));
                caption = value;
            }
            void SetWidth(tjs_int w) override { SetSize(w, height); }
            void SetHeight(tjs_int h) override { SetSize(width, h); }
            void SetSize(tjs_int w, tjs_int h) override {
                check(SDL_SetWindowSize(window.get(), w, h));
                width = w;
                height = h;
            }
            void GetSize(tjs_int &w, tjs_int &h) override {
                w = width;
                h = height;
            }
            tjs_int GetWidth() const override { return width; }
            tjs_int GetHeight() const override { return height; }
            void GetWinSize(tjs_int &w, tjs_int &h) override { GetSize(w, h); }
            void SetPosition(tjs_int x, tjs_int y) override {
                check(SDL_SetWindowPosition(window.get(), x, y));
            }
            void SetLeft(tjs_int x) override { SetPosition(x, GetTop()); }
            void SetTop(tjs_int y) override { SetPosition(GetLeft(), y); }
            tjs_int GetLeft() const override {
                int x;
                check(SDL_GetWindowPosition(window.get(), &x, nullptr));
                return x;
            }
            tjs_int GetTop() const override {
                int y;
                check(SDL_GetWindowPosition(window.get(), nullptr, &y));
                return y;
            }
            void SetMinSize(tjs_int w, tjs_int h) override {
                check(SDL_SetWindowMinimumSize(window.get(), w, h));
            }
            void SetMaxSize(tjs_int w, tjs_int h) override {
                check(SDL_SetWindowMaximumSize(window.get(), w, h));
            }
            void SetMinWidth(tjs_int w) override {
                SetMinSize(w, GetMinHeight());
            }
            void SetMinHeight(tjs_int h) override {
                SetMinSize(GetMinWidth(), h);
            }
            void SetMaxWidth(tjs_int w) override {
                SetMaxSize(w, GetMaxHeight());
            }
            void SetMaxHeight(tjs_int h) override {
                SetMaxSize(GetMaxWidth(), h);
            }
            tjs_int GetMinWidth() const override {
                int w;
                check(SDL_GetWindowMinimumSize(window.get(), &w, nullptr));
                return w;
            }
            tjs_int GetMinHeight() const override {
                int h;
                check(SDL_GetWindowMinimumSize(window.get(), nullptr, &h));
                return h;
            }
            tjs_int GetMaxWidth() const override {
                int w;
                check(SDL_GetWindowMaximumSize(window.get(), &w, nullptr));
                return w;
            }
            tjs_int GetMaxHeight() const override {
                int h;
                check(SDL_GetWindowMaximumSize(window.get(), nullptr, &h));
                return h;
            }
            void SetBorderStyle(tTVPBorderStyle style) override {
                if(style < bsNone || style > bsSizeToolWin)
                    TVPThrowExceptionMessage(
                        TJS_W("Invalid window border style"));
                check(SDL_SetWindowBordered(window.get(), style != bsNone));
                check(SDL_SetWindowResizable(window.get(),
                                             style == bsSizeable ||
                                                 style == bsSizeToolWin));
                borderStyle = style;
            }
            tTVPBorderStyle GetBorderStyle() const override {
                return borderStyle;
            }
            void SetStayOnTop(bool enabled) override {
                check(SDL_SetWindowAlwaysOnTop(window.get(), enabled));
            }
            bool GetStayOnTop() const override {
                return SDL_GetWindowFlags(window.get()) &
                    SDL_WINDOW_ALWAYS_ON_TOP;
            }
            void SetFullScreenMode(bool enabled) override {
                check(SDL_SetWindowFullscreen(window.get(), enabled));
                dirty = true;
            }
            bool GetFullScreenMode() const override {
                return SDL_GetWindowFlags(window.get()) & SDL_WINDOW_FULLSCREEN;
            }
            void SetFocusable(bool enabled) override {
                check(SDL_SetWindowFocusable(window.get(), enabled));
            }
            bool GetFocusable() const override {
                return !(SDL_GetWindowFlags(window.get()) &
                         SDL_WINDOW_NOT_FOCUSABLE);
            }
            void SetMouseCursorState(tTVPMouseCursorState state) override {
                MouseCursorState = state;
                UpdateCursorVisibility();
            }
            void HideMouseCursor() override {
                if(MouseCursorState == mcsVisible)
                    SetMouseCursorState(mcsTempHidden);
            }
            void SetZoom(tjs_int numer, tjs_int denom) override {
                if(numer <= 0 || denom <= 0)
                    TVPThrowExceptionMessage(
                        TJS_W("Window zoom must be positive"));
                const auto scaledWidth =
                    static_cast<std::int64_t>(layerWidth) * numer / denom;
                const auto scaledHeight =
                    static_cast<std::int64_t>(layerHeight) * numer / denom;
                if(scaledWidth < 1 || scaledHeight < 1 ||
                   scaledWidth > std::numeric_limits<int>::max() ||
                   scaledHeight > std::numeric_limits<int>::max())
                    TVPThrowExceptionMessage(
                        TJS_W("Window zoom is out of range"));
                SetSize(static_cast<int>(scaledWidth),
                        static_cast<int>(scaledHeight));
                const auto gcd = std::gcd(numer, denom);
                ZoomNumer = numer / gcd;
                ZoomDenom = denom / gcd;
            }
            void UpdateDrawBuffer(iTVPTexture2D *source) override {
                if(!source)
                    return;
                const auto presentation = source->GetPresentationFrame();
                const auto *frame =
                    std::get_if<render::RGBAFrame>(&presentation);
                if(!frame)
                    throw std::runtime_error(
                        "SDL3 requires a software RGBA frame");
                if(!frame->width || !frame->height)
                    return;
                if(frame->width > std::numeric_limits<int>::max() / 4u ||
                   frame->height >
                       static_cast<unsigned>(std::numeric_limits<int>::max()))
                    throw std::runtime_error("SDL3 frame is too large");
                if(!texture || textureWidth != frame->width ||
                   textureHeight != frame->height) {
                    TexturePtr replacement(
                        SDL_CreateTexture(renderer.get(),
                                          SDL_PIXELFORMAT_RGBA32,
                                          SDL_TEXTUREACCESS_STREAMING,
                                          frame->width, frame->height),
                        SDL_DestroyTexture);
                    check(replacement != nullptr);
                    check(SDL_SetTextureBlendMode(replacement.get(),
                                                  SDL_BLENDMODE_NONE));
                    check(SDL_SetTextureScaleMode(replacement.get(),
                                                  SDL_SCALEMODE_NEAREST));
                    texture = std::move(replacement);
                    textureWidth = frame->width;
                    textureHeight = frame->height;
                }
                void *pixels;
                int pitch;
                check(SDL_LockTexture(texture.get(), nullptr, &pixels, &pitch));
                auto *row = static_cast<unsigned char *>(pixels);
                for(unsigned y = 0; y < frame->height; ++y)
                    std::memcpy(row + static_cast<std::ptrdiff_t>(pitch) * y,
                                frame->Row(y), 4u * frame->width);
                SDL_UnlockTexture(texture.get());
                dirty = true;
            }
            void InvalidateClose() override {
                if(focusedWindow == this) {
                    focusedWindow = nullptr;
                    input::sharedState().releaseAll();
                    if(!SDL_CursorVisible())
                        SDL_ShowCursor();
                }
                native = nullptr;
                texture.reset();
                renderer.reset();
                window.reset();
            }
            bool GetWindowActive() override {
                return GetFormEnabled() && focused && !minimized;
            }
            void Close() override {
                if(!Live() || closePending)
                    return;
                auto lifetime = shared_from_this();
                auto *owner = native->GetOwnerNoAddRef();
                tTJSVariant canClose(true);
                static ttstr eventName(TJS_W("onCloseQuery"));
                closePending = programClosing = true;
                try {
                    TVPPostEvent(owner, owner, eventName, 0, TVP_EPT_IMMEDIATE,
                                 1, &canClose);
                } catch(...) {
                    closePending = programClosing = false;
                    throw;
                }
                closePending = programClosing = false;
            }
            void RequestClose() {
                if(!GetFormEnabled() || closePending)
                    return;
                closePending = true;
                TVPPostInputEvent(new tTVPOnCloseInputEvent(native));
            }
            void OnCloseQueryCalled(bool allowed) override {
                if(!closePending)
                    return;
                closePending = false;
                if(allowed && native) {
                    if(modalWindow == this || (!programClosing && !Main())) {
                        SetVisible(false);
                        return;
                    }
                    auto *owner = native->GetOwnerNoAddRef();
                    owner->Invalidate(0, nullptr, nullptr, owner);
                }
            }
            void InternalKeyDown(tjs_uint16 key, tjs_uint32 shift) override {
                TVPPostInputEvent(
                    new tTVPOnKeyDownInputEvent(native, key, shift));
            }
            void OnKeyUp(tjs_uint16 key, int shift) override {
                TVPPostInputEvent(
                    new tTVPOnKeyUpInputEvent(native, key, shift));
            }
            void OnKeyPress(tjs_uint16 key, int, bool, bool) override {
                TVPPostInputEvent(new tTVPOnKeyPressInputEvent(native, key));
            }
            tTVPImeMode GetDefaultImeMode() const override {
                return defaultIme;
            }
            void SetDefaultImeMode(tTVPImeMode mode, bool reset) override {
                defaultIme = mode;
                if(reset)
                    ResetImeMode();
            }
            void SetImeMode(tTVPImeMode mode) override {
                if(mode == imDontCare)
                    return;
                imeMode = mode;
                UpdateTextInput();
            }
            void ResetImeMode() override { SetImeMode(defaultIme); }
            void UpdateWindow(tTVPUpdateType) override {
                RequestRepaint();
                TVPDeliverWindowUpdateEvents();
            }
            void SetVisibleFromScript(bool value) override {
                SetVisible(value);
            }
            void SetUseMouseKey(bool enabled) override {
                if(enabled)
                    TVPThrowExceptionMessage(
                        TJS_W("SDL3 does not support mouse keys"));
            }
            bool GetUseMouseKey() const override { return false; }
            void ResetMouseVelocity() override { mouseVelocity.clear(); }
            void ResetTouchVelocity(tjs_int) override {}
            bool GetMouseVelocity(float &x, float &y,
                                  float &speed) const override {
                if(!mouseVelocity.getVelocity(x, y)) {
                    speed = 0;
                    return false;
                }
                speed = std::hypot(x, y);
                return true;
            }
            void TickBeat() override {}

            void Present() {
                if(!Live() || !visible || minimized || !dirty)
                    return;
                check(SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255));
                check(SDL_RenderClear(renderer.get()));
                if(texture) {
                    const SDL_FRect crop{ 0, 0,
                                          static_cast<float>(std::min<unsigned>(
                                              layerWidth, textureWidth)),
                                          static_cast<float>(std::min<unsigned>(
                                              layerHeight, textureHeight)) };
                    check(SDL_RenderTexture(renderer.get(), texture.get(),
                                            &crop, nullptr));
                }
                check(SDL_RenderPresent(renderer.get()));
                dirty = false;
            }

            void Handle(const SDL_Event &event) {
                if(!Live())
                    return;
                const auto id = SDL_GetWindowID(window.get());
                switch(event.type) {
                    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                        if(event.window.windowID == id)
                            RequestClose();
                        break;
                    case SDL_EVENT_WINDOW_FOCUS_GAINED:
                    case SDL_EVENT_WINDOW_FOCUS_LOST:
                        if(event.window.windowID == id)
                            SetFocused(event.type ==
                                       SDL_EVENT_WINDOW_FOCUS_GAINED);
                        break;
                    case SDL_EVENT_WINDOW_SHOWN:
                    case SDL_EVENT_WINDOW_HIDDEN:
                        if(event.window.windowID == id)
                            UpdateVisibility(
                                !(SDL_GetWindowFlags(window.get()) &
                                  SDL_WINDOW_HIDDEN));
                        break;
                    case SDL_EVENT_WINDOW_MINIMIZED:
                    case SDL_EVENT_WINDOW_MAXIMIZED:
                    case SDL_EVENT_WINDOW_RESTORED:
                        if(event.window.windowID == id) {
                            minimized =
                                event.type == SDL_EVENT_WINDOW_MINIMIZED;
                            SetFocused(SDL_GetWindowFlags(window.get()) &
                                       SDL_WINDOW_INPUT_FOCUS);
                            dirty = true;
                        }
                        break;
                    case SDL_EVENT_WINDOW_RESIZED:
                        if(event.window.windowID == id) {
                            width = event.window.data1;
                            height = event.window.data2;
                            dirty = true;
                            UpdateAttentionPoint();
                            TVPPostInputEvent(
                                new tTVPOnResizeInputEvent(native));
                        }
                        break;
                    case SDL_EVENT_WINDOW_EXPOSED:
                    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
                    case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
                    case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
                        if(event.window.windowID == id) {
                            if(!texture)
                                RequestRepaint();
                            dirty = true;
                            UpdateAttentionPoint();
                        }
                        break;
                    case SDL_EVENT_KEY_DOWN:
                    case SDL_EVENT_KEY_UP:
                        if(event.key.windowID == id && GetWindowActive())
                            HandleKey(event.key);
                        break;
                    case SDL_EVENT_TEXT_INPUT:
                        if(event.text.windowID == id && GetWindowActive() &&
                           SDL_TextInputActive(window.get())) {
                            if(auto units =
                                   input::decodeTextInput(event.text.text))
                                for(auto unit : *units)
                                    OnKeyPress(unit, 0, false, false);
                        }
                        break;
                    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
                        if(event.window.windowID == id && !PointerHeld())
                            UpdatePointerInside(false);
                        break;
                    case SDL_EVENT_MOUSE_MOTION:
                        if(event.motion.windowID == id && GetWindowActive()) {
                            const bool inside =
                                SetPointer(event.motion.x, event.motion.y);
                            if(MouseCursorState == mcsTempHidden)
                                SetMouseCursorState(mcsVisible);
                            TrackLeftDrag(event.motion.x, event.motion.y);
                            if(inside || PointerHeld())
                                TVPPostInputEvent(
                                    new tTVPOnMouseMoveInputEvent(
                                        native, mouseX, mouseY, ShiftFlags()),
                                    TVP_EPT_DISCARDABLE);
                        }
                        break;
                    case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    case SDL_EVENT_MOUSE_BUTTON_UP:
                        if(event.button.windowID == id && GetWindowActive())
                            HandleButton(event.button);
                        break;
                    case SDL_EVENT_MOUSE_WHEEL:
                        if(event.wheel.windowID == id && GetWindowActive())
                            HandleWheel(event.wheel);
                        break;
                }
            }

        private:
            void UpdateVisibility(bool shown) {
                if(visible == shown)
                    return;
                visible = shown;
                if(visible)
                    RequestRepaint();
                SetFocused(SDL_GetWindowFlags(window.get()) &
                           SDL_WINDOW_INPUT_FOCUS);
                dirty = true;
            }

            void SetFocused(bool active) {
                active = active && GetFormEnabled() && !minimized;
                if(focused == active)
                    return;
                if(active && focusedWindow && focusedWindow != this)
                    focusedWindow->SetFocused(false);
                focused = active;
                if(active) {
                    focusedWindow = this;
                } else if(focusedWindow == this) {
                    focusedWindow = nullptr;
                    ReleaseInput();
                }
                UpdateCursorVisibility();
                UpdateTextInput();
                TVPPostInputEvent(
                    new tTVPOnWindowActivateEvent(native, active));
            }

            void UpdateCursorVisibility() {
                const bool show = !focused || MouseCursorState == mcsVisible;
                if(SDL_CursorVisible() != show)
                    check(show ? SDL_ShowCursor() : SDL_HideCursor());
            }

            input::PointerPixel WindowPoint(float x, float y) const {
                input::PointerPosition point;
                check(SDL_RenderCoordinatesToWindow(renderer.get(), x, y,
                                                    &point.x, &point.y));
                return input::pointerPixel(point);
            }

            void UpdateAttentionPoint() {
                if(!SDL_TextInputActive(window.get()))
                    return;
                if(!attention) {
                    check(SDL_SetTextInputArea(window.get(), nullptr, 0));
                    return;
                }
                const auto &point = *attention;
                const auto start = WindowPoint(point.x, point.y);
                const auto end = WindowPoint(point.x + float(point.w),
                                             point.y + float(point.h));
                const SDL_Rect area{ start.x, start.y,
                                     std::max(1, end.x - start.x),
                                     std::max(1, end.y - start.y) };
                check(SDL_SetTextInputArea(window.get(), &area, 0));
            }

            void UpdateTextInput() {
                const bool requested = GetWindowActive() &&
                    imeMode != imDisable && imeMode != imClose;
                if(requested != SDL_TextInputActive(window.get())) {
                    check(requested ? SDL_StartTextInput(window.get())
                                    : SDL_StopTextInput(window.get()));
                }
                if(requested)
                    UpdateAttentionPoint();
            }

            void RequestRepaint() {
                if(native) {
                    native->NotifyWindowExposureToLayer(
                        tTVPRect(0, 0, layerWidth, layerHeight));
                    native->RequestUpdate();
                }
            }

            static unsigned ShiftFlags() {
                return input::legacyShiftFlags(input::sharedState());
            }
            static bool PointerHeld() {
                const auto &state = input::sharedState();
                return std::any_of(
                    buttonKeys.begin(), buttonKeys.end(),
                    [&](unsigned key) { return state.isDown(key); });
            }
            void UpdatePointerInside(bool inside) {
                if(pointerInside == inside)
                    return;
                pointerInside = inside;
                if(inside) {
                    TVPPostInputEvent(new tTVPOnMouseEnterInputEvent(native));
                } else {
                    TVPPostInputEvent(new tTVPOnMouseLeaveInputEvent(native));
                    TVPPostInputEvent(
                        new tTVPOnMouseOutOfWindowInputEvent(native));
                }
            }
            bool SetPointer(float x, float y) {
                input::PointerPosition position;
                check(SDL_RenderCoordinatesFromWindow(
                    renderer.get(), x, y, &position.x, &position.y));
                const auto pixel = input::pointerPixel(position);
                mouseX = pixel.x;
                mouseY = pixel.y;
                mouseVelocity.addMovement(TVPGetRoughTickCount32(), position.x,
                                          position.y);
                const bool inside = position.x >= 0 && position.y >= 0 &&
                    position.x < layerWidth && position.y < layerHeight;
                UpdatePointerInside(inside);
                return inside;
            }
            void TrackLeftDrag(float x, float y) {
                // Measure in native window points, independent of the layer
                // scale.
                if(leftPress &&
                   (std::abs(x - leftPress->x) > 4 ||
                    std::abs(y - leftPress->y) > 4))
                    leftPress.reset();
            }
            void HandleKey(const SDL_KeyboardEvent &event) {
                const auto scancode = static_cast<unsigned>(event.scancode);
                if(!scancode || scancode >= heldKeys.size())
                    return;
                auto &key = heldKeys[scancode];
                auto &state = input::sharedState();
                if(event.down) {
                    if(event.repeat) {
                        if(key)
                            InternalKeyDown(key, ShiftFlags() | ssRepeat);
                        return;
                    }
                    if(key)
                        return;
                    key = virtualKey(event);
                    if(key && !state.isDown(key)) {
                        state.press(key);
                        InternalKeyDown(key, ShiftFlags());
                    }
                } else if(key) {
                    const auto released = key;
                    key = 0;
                    if(std::find(heldKeys.begin(), heldKeys.end(), released) ==
                           heldKeys.end() &&
                       state.release(released))
                        OnKeyUp(released, ShiftFlags());
                }
            }
            void HandleButton(const SDL_MouseButtonEvent &event) {
                unsigned key;
                switch(event.button) {
                    case SDL_BUTTON_LEFT:
                        key = buttonKeys[mbLeft];
                        break;
                    case SDL_BUTTON_RIGHT:
                        key = buttonKeys[mbRight];
                        break;
                    case SDL_BUTTON_MIDDLE:
                        key = buttonKeys[mbMiddle];
                        break;
                    case SDL_BUTTON_X1:
                        key = buttonKeys[mbX1];
                        break;
                    case SDL_BUTTON_X2:
                        key = buttonKeys[mbX2];
                        break;
                    default:
                        return;
                }
                const auto button = *mouseButton(key);
                auto &state = input::sharedState();
                if(event.down == state.isDown(key))
                    return;
                const bool inside = SetPointer(event.x, event.y);
                if(event.down) {
                    if(!inside)
                        return;
                    state.press(key);
                    if(button == mbLeft)
                        leftPress = SDL_FPoint{ event.x, event.y };
                    TVPPostInputEvent(new tTVPOnMouseDownInputEvent(
                        native, mouseX, mouseY, button,
                        ShiftFlags() | (event.clicks == 2 ? ssDouble : 0)));
                } else {
                    state.release(key);
                    if(button == mbLeft) {
                        TrackLeftDrag(event.x, event.y);
                        if(inside && leftPress) {
                            if(event.clicks == 2)
                                TVPPostInputEvent(
                                    new tTVPOnDoubleClickInputEvent(
                                        native, mouseX, mouseY));
                            else
                                TVPPostInputEvent(new tTVPOnClickInputEvent(
                                    native, mouseX, mouseY));
                        }
                        leftPress.reset();
                    }
                    TVPPostInputEvent(new tTVPOnMouseUpInputEvent(
                        native, mouseX, mouseY, button, ShiftFlags()));
                }
            }
            void HandleWheel(const SDL_MouseWheelEvent &event) {
                if(!SetPointer(event.mouse_x, event.mouse_y) ||
                   !std::isfinite(event.y))
                    return;
                const double direction =
                    event.direction == SDL_MOUSEWHEEL_FLIPPED ? -1 : 1;
                const double raw = direction * event.y * 120 + wheelRemainder;
                const auto delta = static_cast<int>(
                    std::clamp(raw, double(std::numeric_limits<int>::min()),
                               double(std::numeric_limits<int>::max())));
                wheelRemainder = raw - std::trunc(raw);
                if(delta)
                    TVPPostInputEvent(new tTVPOnMouseWheelInputEvent(
                        native, ShiftFlags(), delta, mouseX, mouseY));
            }
            void ReleaseInput() {
                heldKeys.fill(0);
                leftPress.reset();
                wheelRemainder = 0;
                ResetMouseVelocity();
                for(auto key : input::sharedState().releaseAll()) {
                    if(const auto button = mouseButton(key)) {
                        TVPPostInputEvent(new tTVPOnMouseUpInputEvent(
                            native, mouseX, mouseY, *button, ShiftFlags(),
                            true));
                    } else {
                        TVPPostInputEvent(new tTVPOnKeyUpInputEvent(
                            native, key, ShiftFlags(), true));
                    }
                }
                native->PostReleaseCaptureEvent();
                UpdatePointerInside(false);
            }
        };

        struct Session {
            VideoSubsystem video;
            std::vector<std::shared_ptr<Window>> windows;
            bool startedWindow = false;
            bool foreground = false;
        };

        Session *session = nullptr;

        void PumpFrame() {
            auto &windows = session->windows;
            SDL_Event event;
            if(SDL_WaitEventTimeout(&event, 16)) {
                do {
                    if(event.type == SDL_EVENT_QUIT) {
                        const auto main = std::find_if(
                            windows.begin(), windows.end(),
                            [](const auto &window) { return window->Main(); });
                        if(modalWindow)
                            modalWindow->RequestClose();
                        else if(main != windows.end())
                            (*main)->RequestClose();
                        else if(focusedWindow)
                            focusedWindow->RequestClose();
                    } else {
                        for(const auto &window : windows)
                            window->Handle(event);
                    }
                } while(SDL_PollEvent(&event));
            }
            auto *keyboardFocus = SDL_GetKeyboardFocus();
            const bool active =
                std::any_of(windows.begin(), windows.end(),
                            [keyboardFocus](const auto &window) {
                                return window->Owns(keyboardFocus);
                            });
            if(session->foreground != active) {
                session->foreground = active;
                if(active)
                    TVPOnHostForeground();
                else
                    TVPOnHostBackground();
            }
            TVPUpdateEngineFrame();
            for(const auto &window : windows)
                window->Present();
            windows.erase(std::remove_if(windows.begin(), windows.end(),
                                         [](const auto &window) {
                                             return !window->Live();
                                         }),
                          windows.end());
        }
    } // namespace

    bool Active() { return session != nullptr; }

    iWindowLayer *CreateWindow(tTJSNI_Window *owner) {
        auto window = std::make_shared<Window>(owner);
        session->windows.push_back(window);
        session->startedWindow = true;
        return window.get();
    }

    bool GetScreenSize(int &width, int &height) {
        const auto *mode = SDL_GetDesktopDisplayMode(SDL_GetPrimaryDisplay());
        if(!mode)
            return false;
        width = mode->w;
        height = mode->h;
        return true;
    }

    int Run(const std::string &game) {
        int result = 1;
        try {
            Session host;
            session = &host;
            try {
                TVPMainThreadID = std::this_thread::get_id();
                TVPInitNativeDialogBackend();
                LocaleConfigManager::GetInstance()->Initialize(
                    TVPGetCurrentLanguage());
                TVPApplyRuntimePreferenceDefaults(
                    *GlobalConfigManager::GetInstance(), false);
                IndividualConfigManager::GetInstance()->UsePreferenceAt(
                    std::filesystem::u8path(game).parent_path().u8string());
                Application->StartApplication(ttstr(game));
                while(!host.windows.empty())
                    PumpFrame();
                if(Application->IsTarminate())
                    TVPUpdateEngineFrame();
                if(host.startedWindow)
                    result = 0;
                else
                    std::fprintf(
                        stderr, "krkr2: SDL3 startup created no game window\n");
            } catch(const std::exception &error) {
                std::fprintf(stderr, "krkr2: %s\n", error.what());
            }
            // Invalidate engine windows before destroying their host forms.
            TVPSystemUninit();
        } catch(const std::exception &error) {
            std::fprintf(stderr, "krkr2: %s\n", error.what());
            result = 1;
        }
        session = nullptr;
        focusedWindow = modalWindow = nullptr;
        return result;
    }
} // namespace krkr::sdl
