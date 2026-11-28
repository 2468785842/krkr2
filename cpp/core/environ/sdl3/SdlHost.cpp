#include "SdlHost.h"
#include "SdlGamepads.h"
#include "NativeDialogs.h"
#include "resources/LocalAssetReader.h"

#include <SDL3/SDL.h>

#include "tjsCommHead.h"
#include "Application.h"
#include "ConfigManager/GlobalConfigManager.h"
#include "ConfigManager/IndividualConfigManager.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "ConfigManager/PreferenceDefaults.h"
#include "ConfigManager/PreferenceStorage.h"
#include "DialogBackend.h"
#include "EventIntf.h"
#include "HostFrame.h"
#include "FrameStep.h"
#include "HostLifecycle.h"
#include "InputModifiers.h"
#include "InputCancellation.h"
#include "InputState.h"
#include "MsgIntf.h"
#include "PointerCoordinates.h"
#include "RenderManager.h"
#include "SysInitIntf.h"
#include "StorageIntf.h"
#include "TextInput.h"
#include "TickCount.h"
#include "TVPLog.h"
#include "VelocityTracker.h"
#include "WindowImpl.h"
#include "WindowLayer.h"
#include "tvpfontstruc.h"
#include "tjsArray.h"
#include "tvpinputdefs.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstring>
#include <cstdlib>
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
#if KRKR2_ENABLE_UI_TEST_DRIVER
void TVPUpdateSDLGamepadContract(const std::filesystem::path &request);
bool TVPSubmitSDLTouchContract(const std::filesystem::path &request,
                               SDL_WindowID window);
#endif

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

        bool UsesCocoa() {
            const auto *driver = SDL_GetCurrentVideoDriver();
            return driver && std::strcmp(driver, "cocoa") == 0;
        }

        bool UsesAndroid() {
            const auto *driver = SDL_GetCurrentVideoDriver();
            return driver && std::strcmp(driver, "android") == 0;
        }

        struct VideoSubsystem {
            VideoSubsystem() {
                // A native window close is resolved by the script query.
                check(
                    SDL_SetHintWithPriority(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE,
                                            "0", SDL_HINT_OVERRIDE));
                check(SDL_SetHintWithPriority(SDL_HINT_MOUSE_TOUCH_EVENTS, "0",
                                              SDL_HINT_OVERRIDE));
#ifdef __ANDROID__
                // Receive lifecycle events before the SDL loop waits for
                // resume.
                SDL_SetHint(SDL_HINT_ANDROID_BLOCK_ON_PAUSE, "0");
                SDL_SetHint(SDL_HINT_ORIENTATIONS,
                            "LandscapeLeft LandscapeRight");
#endif
                check(SDL_InitSubSystem(SDL_INIT_VIDEO));
                // X11 can use a native software surface. Its implicit GPU
                // framebuffer produces black native capture under Xvfb.
                if(!UsesWayland() && !UsesCocoa() && !UsesAndroid())
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

            struct VideoTrack {
                video::FrameCallback advance;
                TexturePtr picture{ nullptr, SDL_DestroyTexture };
                SDL_FRect bounds{};
                bool visible = true;
                bool pictureVisible = true;
            };

            struct Contact {
                SDL_TouchID device;
                SDL_FingerID finger;
                tjs_int id;
                SDL_FPoint start;
                SDL_FPoint position;
            };

            class TouchInput final : public tTVPBaseInputEvent {
                inline static tTVPUniqueTagForInputEvent tag;
                std::weak_ptr<Window> owner;
                std::optional<SDL_TouchFingerEvent> event;

            public:
                TouchInput(Window &window,
                           std::optional<SDL_TouchFingerEvent> sample) :
                    tTVPBaseInputEvent(window.native, tag),
                    owner(window.shared_from_this()), event(sample) {}

                void Deliver() const override {
                    if(auto window = owner.lock(); window && window->Live()) {
                        if(event)
                            window->DeliverTouch(*event);
                        else
                            window->CancelTouches();
                    }
                }
            };

            class VideoPresentation final : public video::Presentation {
                std::weak_ptr<Window> owner;
                std::shared_ptr<VideoTrack> track;

            public:
                VideoPresentation(Window &window,
                                  video::FrameCallback onFrame) :
                    owner(window.shared_from_this()),
                    track(std::make_shared<VideoTrack>()) {
                    track->advance = std::move(onFrame);
                    window.videos.push_back(track);
                }

                ~VideoPresentation() override {
                    if(auto window = owner.lock()) {
                        track->advance = {};
                        track->picture.reset();
                        auto &videos = window->videos;
                        videos.erase(
                            std::remove(videos.begin(), videos.end(), track),
                            videos.end());
                        window->dirty = true;
                    }
                }

                void SetVisible(bool visible) override {
                    if(track->visible == visible)
                        return;
                    track->visible = visible;
                    if(auto window = owner.lock())
                        window->dirty = true;
                }

                void SetPictureVisible(bool visible) override {
                    if(track->pictureVisible == visible)
                        return;
                    track->pictureVisible = visible;
                    if(auto window = owner.lock())
                        window->dirty = true;
                }

                void Present(const video::Yuv420Frame &frame,
                             const video::Bounds &bounds) override {
                    auto window = owner.lock();
                    if(!window || !window->Live())
                        return;
                    auto &picture = track->picture;
                    if(!picture || picture->w != frame.width ||
                       picture->h != frame.height) {
                        picture.reset(SDL_CreateTexture(
                            window->renderer.get(), SDL_PIXELFORMAT_IYUV,
                            SDL_TEXTUREACCESS_STREAMING, frame.width,
                            frame.height));
                        check(picture != nullptr);
                    }
                    check(SDL_UpdateYUVTexture(
                        picture.get(), nullptr, frame.y, frame.width, frame.u,
                        (frame.width + 1) / 2, frame.v, (frame.width + 1) / 2));
                    track->bounds = { float(bounds.left), float(bounds.top),
                                      float(bounds.width),
                                      float(bounds.height) };
                    window->dirty = true;
                }
            };

            tTJSNI_Window *native;
            Gamepads &gamepads;
            WindowPtr window{ nullptr, SDL_DestroyWindow };
            RendererPtr renderer{ nullptr, SDL_DestroyRenderer };
            TexturePtr texture{ nullptr, SDL_DestroyTexture };
            std::vector<std::shared_ptr<VideoTrack>> videos;
            std::vector<ttstr> droppedFiles;
            Uint64 lastVideoTick = SDL_GetTicksNS();
            std::string caption = "krkr2";
            int width = 640, height = 480;
            int layerWidth = 640, layerHeight = 480;
            unsigned textureWidth = 0, textureHeight = 0;
            int mouseX = 0, mouseY = 0;
            bool visible = false, focused = false, minimized = false;
            bool pointerInside = false;
            bool dirty = true, closePending = false;
            const bool wayland = UsesWayland();
            const bool android = UsesAndroid();
#ifdef __ANDROID__
            int requestedLeft = 0, requestedTop = 0;
            const bool showFps =
                IndividualConfigManager::GetInstance()->GetValue<bool>(
                    "showfps", TVPPreferenceDefaults::value_showfps);
            const float cursorScale = std::clamp(
                IndividualConfigManager::GetInstance()->GetValue<float>(
                    "vcursor_scale",
                    TVPPreferenceDefaults::value_vcursor_scale),
                0.0f, 1.0f);
            Uint64 fpsSample = SDL_GetTicksNS();
            unsigned framesPresented = 0, displayedFps = 0;
#endif
            bool drawableCommitPending = wayland;
            bool programClosing = false;
            std::array<tjs_uint16, SDL_SCANCODE_COUNT> heldKeys{};
            std::array<bool, SDL_SCANCODE_COUNT> mouseKeyBindings{};
            std::bitset<0x200> padMouseKeys;
            unsigned heldPadKeys = 0;
            std::array<bool, 5> physicalButtons{};
            std::array<bool, 2> keyboardButtons{};
            bool touchMousePressed = false;
            bool mouseKeysEnabled = false;
            bool showKeyboardCursor = false;
            std::optional<input::PointerPixel> keyboardWarp;
            Uint64 nextMouseKeyStep = 0;
            std::optional<SDL_FPoint> leftPress;
            float wheelRemainder = 0;
            tTVPImeMode defaultIme = imDisable, imeMode = imDisable;
            std::optional<SDL_Rect> attention;
            VelocityTracker mouseVelocity;
            VelocityTrackers touchVelocity;
            std::vector<Contact> contacts;
            tjs_int nextContact = 0;
            bool touchEnabled = false;
            bool scaling = false;
            bool rotating = false;
            tTVPBorderStyle borderStyle = bsSizeable;

        public:
            Window(tTJSNI_Window *owner, Gamepads &pads) :
                native(owner), gamepads(pads) {
                SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE |
                    SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
                if(android)
                    flags |= SDL_WINDOW_FULLSCREEN;
                window.reset(
                    SDL_CreateWindow(caption.c_str(), width, height, flags));
                check(window != nullptr);
                // Android, Wayland and Cocoa use GPU presentation to upload
                // the engine's CPU-rendered frames.
                const char *driver = "software";
                if(UsesCocoa())
                    driver = "metal";
                else if(wayland)
                    driver = "opengles2,opengl";
                else if(android)
                    driver = "opengles2";
                renderer.reset(SDL_CreateRenderer(window.get(), driver));
                check(renderer != nullptr);
                SetPaintBoxSize(layerWidth, layerHeight);
#ifdef __ANDROID__
                SetUseMouseKey(
                    IndividualConfigManager::GetInstance()->GetValue<bool>(
                        "virtual_mouse", false));
#endif
                G_CoreLog.info("SDL3 video={}, renderer={}",
                               SDL_GetCurrentVideoDriver(),
                               SDL_GetRendererName(renderer.get()));
            }

            bool Live() const { return native != nullptr; }
            bool Main() const { return Live() && native->IsMainWindow(); }
#if KRKR2_ENABLE_UI_TEST_DRIVER
            SDL_WindowID Id() const { return SDL_GetWindowID(window.get()); }
#endif
            bool Owns(SDL_Window *candidate) const {
                return candidate && candidate == window.get();
            }
            std::unique_ptr<video::Presentation>
            CreateVideoPresentation(video::FrameCallback onFrame) {
                return std::make_unique<VideoPresentation>(*this,
                                                           std::move(onFrame));
            }
            void SetPaintBoxSize(tjs_int w, tjs_int h) override {
                if(w <= 0 || h <= 0)
                    return;
                layerWidth = w;
                layerHeight = h;
                UpdatePresentation();
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
                // Android owns the surface size; keep the script window's
                // requested size separately from the native drawable.
                if(!android)
                    check(SDL_SetWindowSize(window.get(), w, h));
                else if(w <= 0 || h <= 0)
                    TVPThrowExceptionMessage(
                        TJS_W("Window size must be positive"));
                width = w;
                height = h;
            }
            void GetSize(tjs_int &w, tjs_int &h) override {
                w = width;
                h = height;
            }
            tjs_int GetWidth() const override { return width; }
            tjs_int GetHeight() const override { return height; }
            void GetWinSize(tjs_int &w, tjs_int &h) override {
                if(android)
                    check(SDL_GetWindowSize(window.get(), &w, &h));
                else
                    GetSize(w, h);
            }
            void SetPosition(tjs_int x, tjs_int y) override {
#ifdef __ANDROID__
                requestedLeft = x;
                requestedTop = y;
#else
                check(SDL_SetWindowPosition(window.get(), x, y));
#endif
            }
            void SetLeft(tjs_int x) override { SetPosition(x, GetTop()); }
            void SetTop(tjs_int y) override { SetPosition(GetLeft(), y); }
            tjs_int GetLeft() const override {
#ifdef __ANDROID__
                return requestedLeft;
#else
                int x;
                check(SDL_GetWindowPosition(window.get(), &x, nullptr));
                return x;
#endif
            }
            tjs_int GetTop() const override {
#ifdef __ANDROID__
                return requestedTop;
#else
                int y;
                check(SDL_GetWindowPosition(window.get(), nullptr, &y));
                return y;
#endif
            }
            void SetMinSize(tjs_int w, tjs_int h) override {
                // SDL records Android size constraints even though the final
                // native resize cannot be performed on this Activity.
                const bool applied =
                    SDL_SetWindowMinimumSize(window.get(), w, h);
                check(applied ||
                      (android && w == GetMinWidth() && h == GetMinHeight()));
            }
            void SetMaxSize(tjs_int w, tjs_int h) override {
                const bool applied =
                    SDL_SetWindowMaximumSize(window.get(), w, h);
                check(applied ||
                      (android && w == GetMaxWidth() && h == GetMaxHeight()));
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
                if(showKeyboardCursor)
                    dirty = true;
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
                    gamepads.CancelInput();
                    input::sharedState().releaseAll();
                    if(!SDL_CursorVisible())
                        SDL_ShowCursor();
                }
                native = nullptr;
                for(const auto &video : videos) {
                    video->advance = {};
                    video->picture.reset();
                }
                videos.clear();
                texture.reset();
                renderer.reset();
                window.reset();
            }
            bool GetWindowActive() override {
                return GetFormEnabled() && focused && !minimized;
            }
            void HandleGamepadKey(const Gamepads::Transition &transition) {
                const auto key = transition.key;
                auto &state = input::sharedState();
                const auto button = input::mouseKeyButton(key);
                if(transition.down) {
                    state.press(key);
                    state.press(VK_PADANY);
                    ++heldPadKeys;
                    const bool asMouse = mouseKeysEnabled &&
                        (button || (key >= VK_PADLEFT && key <= VK_PADDOWN));
                    padMouseKeys[key] = asMouse;
                    if(!asMouse)
                        InternalKeyDown(key, ShiftFlags());
                    else if(button)
                        PressKeyboardMouseButton(button);
                    else
                        TickBeat();
                } else if(state.release(key)) {
                    if(--heldPadKeys == 0)
                        state.release(VK_PADANY);
                    if(!padMouseKeys[key]) {
                        TVPPostInputEvent(
                            new tTVPOnKeyUpInputEvent(native, key, ShiftFlags(),
                                                      transition.cancellation));
                    } else {
                        padMouseKeys.reset(key);
                        if(button)
                            ReleaseKeyboardButton(button,
                                                  transition.cancellation);
                    }
                }
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
                if(mouseKeysEnabled && input::mouseKeyButton(key))
                    return;
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
                if(mouseKeysEnabled == enabled)
                    return;
                mouseKeysEnabled = enabled;
                nextMouseKeyStep = 0;
                if(enabled)
                    return;
                StopKeyboardCursor();
                for(std::size_t index = 0; index < keyboardButtons.size();
                    ++index)
                    if(std::exchange(keyboardButtons[index], false))
                        UpdateMouseButton(index, 1, true);
            }
            bool GetUseMouseKey() const override { return mouseKeysEnabled; }
            void ResetMouseVelocity() override { mouseVelocity.clear(); }
            void ResetTouchVelocity(tjs_int id) override {
                touchVelocity.end(id);
            }
            void SetEnableTouch(bool enabled) override {
                if(touchEnabled && !enabled)
                    QueueTouchCancellation();
                if(enabled && std::exchange(touchMousePressed, false))
                    UpdateMouseButton(mbLeft, 1, true);
                touchEnabled = enabled;
            }
            bool GetEnableTouch() const override { return touchEnabled; }
            bool HasTouchDevice() const override {
                int count = 0;
                auto *devices = SDL_GetTouchDevices(&count);
                bool supported = false;
                for(int i = 0; devices && i < count; ++i)
                    if(SDL_GetTouchDeviceType(devices[i]) ==
                       SDL_TOUCH_DEVICE_DIRECT) {
                        supported = true;
                        break;
                    }
                SDL_free(devices);
                return supported;
            }
            tjs_int GetTouchPointCount() const override {
                return static_cast<tjs_int>(contacts.size());
            }
            tjs_real GetTouchPointStartX(tjs_int index) const override {
                const auto *contact = ContactAt(index);
                return contact ? contact->start.x : 0;
            }
            tjs_real GetTouchPointStartY(tjs_int index) const override {
                const auto *contact = ContactAt(index);
                return contact ? contact->start.y : 0;
            }
            tjs_real GetTouchPointX(tjs_int index) const override {
                const auto *contact = ContactAt(index);
                return contact ? contact->position.x : 0;
            }
            tjs_real GetTouchPointY(tjs_int index) const override {
                const auto *contact = ContactAt(index);
                return contact ? contact->position.y : 0;
            }
            tjs_int GetTouchPointID(tjs_int index) const override {
                const auto *contact = ContactAt(index);
                return contact ? contact->id : 0;
            }
            bool GetTouchVelocity(tjs_int id, float &x, float &y,
                                  float &speed) const override {
                return touchVelocity.getVelocity(id, x, y, speed);
            }
            bool GetMouseVelocity(float &x, float &y,
                                  float &speed) const override {
                if(!mouseVelocity.getVelocity(x, y)) {
                    speed = 0;
                    return false;
                }
                speed = std::hypot(x, y);
                return true;
            }
            void TickVideos(Uint64 now) {
                const auto elapsed = now - std::exchange(lastVideoTick, now);
                const auto snapshot = videos;
                for(const auto &video : snapshot)
                    if(auto advance = video->advance)
                        advance(static_cast<float>(elapsed) /
                                SDL_NS_PER_SECOND);
            }

            void TickBeat() override {
                if(!mouseKeysEnabled || !GetWindowActive())
                    return;
                const auto now = SDL_GetTicks();
                if(now < nextMouseKeyStep)
                    return;
                const auto down = [&](tjs_uint16 key) {
                    for(std::size_t i = 0; i < heldKeys.size(); ++i)
                        if(mouseKeyBindings[i] && heldKeys[i] == key)
                            return 1;
                    return 0;
                };
                const int step = (ShiftFlags() & ssShift) ? 8 : 2;
                const int dx = ((down(0x27) || padMouseKeys[VK_PADRIGHT]) -
                                (down(0x25) || padMouseKeys[VK_PADLEFT])) *
                    step;
                const int dy = ((down(0x28) || padMouseKeys[VK_PADDOWN]) -
                                (down(0x26) || padMouseKeys[VK_PADUP])) *
                    step;
                if(dx || dy) {
                    nextMouseKeyStep = now + 24;
                    MoveKeyboardCursor(dx, dy);
                }
            }

            void Present(bool swap = true) {
#ifdef __ANDROID__
                if(showFps && swap) {
                    const auto now = SDL_GetTicksNS();
                    const auto elapsed = now - fpsSample;
                    if(elapsed >= SDL_NS_PER_SECOND) {
                        displayedFps =
                            framesPresented * SDL_NS_PER_SECOND / elapsed;
                        framesPresented = 0;
                        fpsSample = now;
                        dirty = true;
                    }
                }
#endif
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
                if(!videos.empty() || showKeyboardCursor) {
                    const SDL_Rect clip{ 0, 0, layerWidth, layerHeight };
                    check(SDL_SetRenderClipRect(renderer.get(), &clip));
                    for(const auto &video : videos) {
                        if(video->visible && video->pictureVisible &&
                           video->picture && video->bounds.w > 0 &&
                           video->bounds.h > 0)
                            check(SDL_RenderTexture(renderer.get(),
                                                    video->picture.get(),
                                                    nullptr, &video->bounds));
                    }
                    if(showKeyboardCursor && MouseCursorState == mcsVisible) {
#ifdef __ANDROID__
                        const float size = 6.0f + cursorScale * 18.0f;
#else
                        constexpr float size = 12.0f;
#endif
                        const SDL_FPoint pointer[] = {
                            { float(mouseX), mouseY + size },
                            { float(mouseX), float(mouseY) },
                            { mouseX + size * (2.0f / 3.0f),
                              mouseY + size * (2.0f / 3.0f) },
                        };
                        check(SDL_SetRenderDrawColor(renderer.get(), 255, 255,
                                                     255, 255));
                        check(SDL_RenderLines(renderer.get(), pointer, 3));
                    }
                    check(SDL_SetRenderClipRect(renderer.get(), nullptr));
                }
#ifdef __ANDROID__
                if(showFps && swap) {
                    check(SDL_SetRenderDrawColor(renderer.get(), 0, 0, 0, 255));
                    const SDL_FRect background{ 0, 0, 88, 14 };
                    check(SDL_RenderFillRect(renderer.get(), &background));
                    check(SDL_SetRenderDrawColor(renderer.get(), 255, 255, 255,
                                                 255));
                    check(SDL_RenderDebugTextFormat(renderer.get(), 3, 3,
                                                    "%u FPS", displayedFps));
                }
#endif
                if(swap) {
                    check(SDL_RenderPresent(renderer.get()));
#ifdef __ANDROID__
                    ++framesPresented;
#endif
                }
                // Mesa may have acquired the previous Wayland buffer before
                // the resize. Commit again with the newly allocated buffer.
                dirty = std::exchange(drawableCommitPending, false);
            }

            bool Capture(const std::string &path) {
                if(!Live())
                    return false;
                dirty = true;
                Present(false);
                std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)>
                    surface(SDL_RenderReadPixels(renderer.get(), nullptr),
                            SDL_DestroySurface);
                const bool saved =
                    surface && SDL_SaveBMP(surface.get(), path.c_str());
                check(SDL_RenderPresent(renderer.get()));
                return saved;
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
                            if(minimized) {
                                droppedFiles.clear();
                                QueueTouchCancellation();
                            }
                            SetFocused(SDL_GetWindowFlags(window.get()) &
                                       SDL_WINDOW_INPUT_FOCUS);
                            dirty = true;
                        }
                        break;
                    case SDL_EVENT_WINDOW_RESIZED:
                        if(event.window.windowID == id) {
                            if(!android) {
                                width = event.window.data1;
                                height = event.window.data2;
                            }
                            dirty = true;
                            drawableCommitPending = wayland;
                            UpdatePresentation();
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
                            drawableCommitPending = wayland;
                            UpdatePresentation();
                        }
                        break;
#ifdef __ANDROID__
                    case SDL_EVENT_WILL_ENTER_BACKGROUND:
                        SetFocused(false);
                        break;
                    case SDL_EVENT_DID_ENTER_FOREGROUND:
                        SetFocused(SDL_GetWindowFlags(window.get()) &
                                   SDL_WINDOW_INPUT_FOCUS);
                        dirty = true;
                        break;
                    case SDL_EVENT_RENDER_DEVICE_RESET:
                        if(event.render.windowID == id) {
                            texture.reset();
                            for(const auto &video : videos)
                                video->picture.reset();
                            RequestRepaint();
                            dirty = true;
                        }
                        break;
#endif
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
                        if(event.motion.windowID == id && GetWindowActive() &&
                           (!touchEnabled ||
                            event.motion.which != SDL_TOUCH_MOUSEID)) {
                            const auto warp = std::exchange(keyboardWarp, {});
                            if(warp && event.motion.x == warp->x &&
                               event.motion.y == warp->y)
                                break;
                            StopKeyboardCursor();
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
                        if(event.button.windowID == id && GetWindowActive() &&
                           (!touchEnabled ||
                            event.button.which != SDL_TOUCH_MOUSEID))
                            HandleButton(event.button);
                        break;
                    case SDL_EVENT_MOUSE_WHEEL:
                        if(event.wheel.windowID == id && GetWindowActive())
                            HandleWheel(event.wheel);
                        break;
                    case SDL_EVENT_DROP_BEGIN:
                    case SDL_EVENT_DROP_FILE:
                    case SDL_EVENT_DROP_COMPLETE:
                        if(event.drop.windowID == id)
                            HandleDrop(event.drop);
                        break;
                    case SDL_EVENT_FINGER_DOWN:
                    case SDL_EVENT_FINGER_MOTION:
                    case SDL_EVENT_FINGER_UP:
                    case SDL_EVENT_FINGER_CANCELED:
                        if(event.tfinger.windowID == id && touchEnabled &&
                           GetFormEnabled() && !minimized) {
                            auto touch = event.tfinger;
                            tjs_int surfaceWidth, surfaceHeight;
                            GetWinSize(surfaceWidth, surfaceHeight);
                            check(SDL_RenderCoordinatesFromWindow(
                                renderer.get(), touch.x * surfaceWidth,
                                touch.y * surfaceHeight, &touch.x, &touch.y));
                            TVPPostInputEvent(new TouchInput(*this, touch));
                        }
                        break;
                }
            }

        private:
            void UpdatePresentation() {
                check(SDL_SetRenderLogicalPresentation(
                    renderer.get(), layerWidth, layerHeight,
                    SDL_LOGICAL_PRESENTATION_LETTERBOX));
                UpdateAttentionPoint();
            }

            void UpdateVisibility(bool shown) {
                if(visible == shown)
                    return;
                visible = shown;
                droppedFiles.clear();
                if(visible) {
                    RequestRepaint();
                    drawableCommitPending = wayland;
                } else
                    QueueTouchCancellation();
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
                    QueueTouchCancellation();
                }
                UpdateCursorVisibility();
                UpdateTextInput();
                TVPPostInputEvent(
                    new tTVPOnWindowActivateEvent(native, active));
            }

            void UpdateCursorVisibility() {
                const bool show = !focused ||
                    (MouseCursorState == mcsVisible && !showKeyboardCursor);
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
            const Contact *ContactAt(tjs_int index) const {
                return index >= 0 &&
                        static_cast<std::size_t>(index) < contacts.size()
                    ? &contacts[index]
                    : nullptr;
            }
            void QueueTouchCancellation() {
                if(touchEnabled || !contacts.empty())
                    TVPPostInputEvent(new TouchInput(*this, std::nullopt));
            }
            void CancelTouches() {
                while(Live() && !contacts.empty()) {
                    const auto contact = contacts.back();
                    contacts.pop_back();
                    native->OnTouchUp(contact.position.x, contact.position.y, 0,
                                      0, contact.id, true);
                }
                scaling = rotating = false;
            }
            void DeliverTouch(const SDL_TouchFingerEvent &event) {
                if(!touchEnabled || !GetFormEnabled())
                    return;
                auto contact =
                    std::find_if(contacts.begin(), contacts.end(),
                                 [&](const Contact &point) {
                                     return point.device == event.touchID &&
                                         point.finger == event.fingerID;
                                 });
                const bool validPosition =
                    std::isfinite(event.x) && std::isfinite(event.y);
                const auto tick =
                    static_cast<tjs_uint32>(event.timestamp / SDL_NS_PER_MS);
                const SDL_FPoint position{ event.x, event.y };
                if(event.type == SDL_EVENT_FINGER_DOWN) {
                    if(contact != contacts.end() || !validPosition ||
                       event.x < 0 || event.y < 0 || event.x >= layerWidth ||
                       event.y >= layerHeight)
                        return;
                    do {
                        nextContact =
                            nextContact == std::numeric_limits<tjs_int>::max()
                            ? 1
                            : nextContact + 1;
                    } while(std::any_of(contacts.begin(), contacts.end(),
                                        [&](const Contact &active) {
                                            return active.id == nextContact;
                                        }));
                    contacts.push_back({ event.touchID, event.fingerID,
                                         nextContact, position, position });
                    touchVelocity.start(nextContact);
                    touchVelocity.update(nextContact, tick, event.x, event.y);
                    native->OnTouchDown(event.x, event.y, 0, 0, nextContact);
                } else if(contact != contacts.end()) {
                    if(validPosition) {
                        contact->position = position;
                        touchVelocity.update(contact->id, tick, position.x,
                                             position.y);
                    }
                    const auto point = *contact;
                    if(event.type == SDL_EVENT_FINGER_MOTION) {
                        if(!validPosition)
                            return;
                        native->OnTouchMove(position.x, position.y, 0, 0,
                                            point.id);
                        if(Live() && touchEnabled)
                            UpdateGestures();
                    } else {
                        contacts.erase(contact);
                        scaling = rotating = false;
                        native->OnTouchUp(
                            point.position.x, point.position.y, 0, 0, point.id,
                            event.type == SDL_EVENT_FINGER_CANCELED);
                    }
                } else {
                    return;
                }
                if(Live() && touchEnabled && contacts.size() >= 2)
                    native->OnMultiTouch();
            }
            void UpdateGestures() {
                if(contacts.size() != 2 ||
                   contacts[0].device != contacts[1].device)
                    return;
                const auto first = contacts[0];
                const auto second = contacts[1];
                const auto sx = second.start.x - first.start.x;
                const auto sy = second.start.y - first.start.y;
                const auto dx = second.position.x - first.position.x;
                const auto dy = second.position.y - first.position.y;
                const auto start = std::hypot(sx, sy);
                const auto distance = std::hypot(dx, dy);
                const auto cx = (first.position.x + second.position.x) * 0.5;
                const auto cy = (first.position.y + second.position.y) * 0.5;
                if(scaling ||
                   std::abs(distance - start) > TouchScaleThreshold) {
                    const auto firstUpdate = !std::exchange(scaling, true);
                    native->OnTouchScaling(start, distance, cx, cy,
                                           firstUpdate ? 1 : 0);
                }
                const auto travel =
                    std::hypot(first.position.x - first.start.x,
                               first.position.y - first.start.y) +
                    std::hypot(second.position.x - second.start.x,
                               second.position.y - second.start.y);
                if(Live() && touchEnabled &&
                   (rotating || travel > TouchRotateThreshold)) {
                    const auto firstUpdate = !std::exchange(rotating, true);
                    native->OnTouchRotate(std::atan2(sy, sx),
                                          std::atan2(dy, dx), distance, cx, cy,
                                          firstUpdate ? 1 : 0);
                }
            }
            void HandleDrop(const SDL_DropEvent &drop) {
                if(!GetFormEnabled() || minimized ||
                   drop.type == SDL_EVENT_DROP_BEGIN) {
                    droppedFiles.clear();
                    return;
                }
                if(drop.type == SDL_EVENT_DROP_FILE) {
                    if(drop.data && *drop.data)
                        droppedFiles.push_back(
                            TVPNormalizeStorageName(ttstr(drop.data)));
                    return;
                }
                auto files = std::exchange(droppedFiles, {});
                if(files.empty())
                    return;
                auto release = [](iTJSDispatch2 *object) { object->Release(); };
                std::unique_ptr<iTJSDispatch2, decltype(release)> array(
                    TJSCreateArrayObject(), release);
                for(std::size_t index = 0; index < files.size(); ++index) {
                    const tTJSVariant path(files[index]);
                    array->PropSetByNum(TJS_MEMBERENSURE, index, &path,
                                        array.get());
                }
                TVPPostInputEvent(new tTVPOnFileDropInputEvent(
                    native, tTJSVariant(array.get(), array.get())));
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
            void StopKeyboardCursor() {
                keyboardWarp.reset();
                if(std::exchange(showKeyboardCursor, false)) {
                    dirty = true;
                    UpdateCursorVisibility();
                }
            }
            void MoveKeyboardCursor(int dx, int dy) {
                const auto x = std::clamp(mouseX + dx, 0, layerWidth - 1);
                const auto y = std::clamp(mouseY + dy, 0, layerHeight - 1);
                if(x == mouseX && y == mouseY && pointerInside)
                    return;
                if(MouseCursorState == mcsTempHidden)
                    SetMouseCursorState(mcsVisible);
                const auto windowPoint = WindowPoint(x, y);
                if(!wayland && !android) {
                    keyboardWarp = windowPoint;
                    SetCursorPos(x, y);
                } else {
                    showKeyboardCursor = true;
                    UpdateCursorVisibility();
                    dirty = true;
                }
                mouseX = x;
                mouseY = y;
                TrackLeftDrag(windowPoint.x, windowPoint.y);
                mouseVelocity.addMovement(TVPGetRoughTickCount32(), x, y);
                UpdatePointerInside(true);
                TVPPostInputEvent(
                    new tTVPOnMouseMoveInputEvent(native, x, y, ShiftFlags()),
                    TVP_EPT_DISCARDABLE);
            }
            void PressKeyboardMouseButton(unsigned button) {
                const auto index =
                    static_cast<std::size_t>(*mouseButton(button));
                MoveKeyboardCursor(0, 0);
                keyboardButtons[index] = true;
                UpdateMouseButton(index, 1);
            }
            void ReleaseKeyboardButton(unsigned button, bool cancelled) {
                const auto index =
                    static_cast<std::size_t>(*mouseButton(button));
                if(padMouseKeys[button == input::key::leftMouse ? VK_PAD1
                                                                : VK_PAD2])
                    return;
                for(std::size_t i = 0; i < heldKeys.size(); ++i)
                    if(mouseKeyBindings[i] &&
                       input::mouseKeyButton(heldKeys[i]) == button)
                        return;
                if(std::exchange(keyboardButtons[index], false))
                    UpdateMouseButton(index, 1, cancelled);
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
                        if(key && !mouseKeyBindings[scancode])
                            InternalKeyDown(key, ShiftFlags() | ssRepeat);
                        return;
                    }
                    if(key)
                        return;
                    key = virtualKey(event);
                    if(key && !state.isDown(key)) {
                        state.press(key);
                        const auto button = input::mouseKeyButton(key);
                        mouseKeyBindings[scancode] = mouseKeysEnabled &&
                            (button || (key >= 0x25 && key <= 0x28));
                        if(!mouseKeyBindings[scancode]) {
                            InternalKeyDown(key, ShiftFlags());
                        } else if(button) {
                            PressKeyboardMouseButton(button);
                        } else {
                            TickBeat();
                        }
                    }
                } else if(key) {
                    const auto released = key;
                    key = 0;
                    const bool consumed =
                        std::exchange(mouseKeyBindings[scancode], false);
                    if(std::find(heldKeys.begin(), heldKeys.end(), released) ==
                           heldKeys.end() &&
                       state.release(released) && !consumed)
                        OnKeyUp(released, ShiftFlags());
                    if(consumed) {
                        // SDL_ResetKeyboard posts artificial releases before
                        // it posts the focus-lost event.
                        const bool cancelled = !event.raw && !event.which;
                        if(const auto button = input::mouseKeyButton(released))
                            ReleaseKeyboardButton(button, cancelled);
                    }
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
                const auto index = static_cast<std::size_t>(button);
                auto &pressed =
                    button == mbLeft && event.which == SDL_TOUCH_MOUSEID
                    ? touchMousePressed
                    : physicalButtons[index];
                if(event.down == pressed)
                    return;
                const bool inside = SetPointer(event.x, event.y);
                if(event.down && !inside)
                    return;
                pressed = event.down;
                if(button == mbLeft)
                    TrackLeftDrag(event.x, event.y);
                const auto previousPress = leftPress;
                UpdateMouseButton(index, event.clicks);
                if(button == mbLeft && event.down && !previousPress &&
                   leftPress)
                    leftPress = SDL_FPoint{ event.x, event.y };
            }
            void UpdateMouseButton(std::size_t index, unsigned clicks,
                                   bool cancelled = false) {
                const auto key = buttonKeys[index];
                const auto button = static_cast<tTVPMouseButton>(index);
                const bool down = physicalButtons[index] ||
                    (index < keyboardButtons.size() &&
                     keyboardButtons[index]) ||
                    (button == mbLeft && touchMousePressed);
                auto &state = input::sharedState();
                if(down == state.isDown(key))
                    return;
                if(down) {
                    state.press(key);
                    if(button == mbLeft) {
                        const auto point = WindowPoint(mouseX, mouseY);
                        leftPress =
                            SDL_FPoint{ float(point.x), float(point.y) };
                    }
                    TVPPostInputEvent(new tTVPOnMouseDownInputEvent(
                        native, mouseX, mouseY, button,
                        ShiftFlags() | (clicks == 2 ? ssDouble : 0)));
                } else {
                    state.release(key);
                    if(button == mbLeft) {
                        if(!cancelled && pointerInside && leftPress) {
                            if(clicks == 2)
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
                        native, mouseX, mouseY, button, ShiftFlags(),
                        cancelled));
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
                gamepads.CancelInput();
                leftPress.reset();
                wheelRemainder = 0;
                StopKeyboardCursor();
                physicalButtons.fill(false);
                keyboardButtons.fill(false);
                touchMousePressed = false;
                nextMouseKeyStep = 0;
                ResetMouseVelocity();
                for(auto key : input::sharedState().releaseAll()) {
                    if(const auto button = mouseButton(key)) {
                        TVPPostInputEvent(new tTVPOnMouseUpInputEvent(
                            native, mouseX, mouseY, *button, ShiftFlags(),
                            true));
                    } else if(key != VK_PADANY) {
                        bool consumed = padMouseKeys[key];
                        for(std::size_t i = 0; i < heldKeys.size(); ++i)
                            consumed |=
                                mouseKeyBindings[i] && heldKeys[i] == key;
                        if(!consumed)
                            TVPPostInputEvent(new tTVPOnKeyUpInputEvent(
                                native, key, ShiftFlags(), true));
                    }
                }
                heldKeys.fill(0);
                mouseKeyBindings.fill(false);
                padMouseKeys.reset();
                heldPadKeys = 0;
                native->PostReleaseCaptureEvent();
                UpdatePointerInside(false);
            }
        };

        struct Session {
            VideoSubsystem video;
            Gamepads gamepads;
            std::vector<std::shared_ptr<Window>> windows;
            bool startedWindow = false;
            bool foreground = false;
#ifdef __ANDROID__
            bool background = false;
            Uint64 frameInterval = 0;
            Uint64 nextFrame = 0;
#endif
            bool ending = false;
#if KRKR2_ENABLE_UI_TEST_DRIVER
            std::filesystem::path gamepadContractRequest;
            std::filesystem::path touchContractRequest;
#endif
        };

        Session *session = nullptr;

        void PumpFrame() {
            PumpNativeDialogs();
            auto &windows = session->windows;
#if KRKR2_ENABLE_UI_TEST_DRIVER
            if(!session->gamepadContractRequest.empty())
                TVPUpdateSDLGamepadContract(session->gamepadContractRequest);
            if(!session->touchContractRequest.empty()) {
                const auto main = std::find_if(
                    windows.begin(), windows.end(),
                    [](const auto &window) { return window->Main(); });
                if(main != windows.end() &&
                   TVPSubmitSDLTouchContract(session->touchContractRequest,
                                             (*main)->Id()))
                    session->touchContractRequest.clear();
            }
#endif
            SDL_Event event;
            std::vector<Gamepads::Transition> padTransitions;
#ifdef __ANDROID__
            const auto start = SDL_GetTicksNS();
            const auto remaining =
                session->nextFrame > start ? session->nextFrame - start : 0;
            const auto timeout = session->background
                ? 100
                : static_cast<int>((remaining + SDL_NS_PER_MS - 1) /
                                   SDL_NS_PER_MS);
#else
            constexpr int timeout = 16;
#endif
            if(SDL_WaitEventTimeout(&event, timeout)) {
                do {
#ifdef __ANDROID__
                    if(event.type == SDL_EVENT_TERMINATING) {
                        session->ending = true;
                        return;
                    }
                    if(event.type == SDL_EVENT_WILL_ENTER_BACKGROUND)
                        session->background = true;
                    if(event.type == SDL_EVENT_DID_ENTER_FOREGROUND)
                        session->background = false;
#endif
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
                        auto *target =
                            focusedWindow && focusedWindow->GetWindowActive()
                            ? focusedWindow
                            : nullptr;
                        if(session->gamepads.Handle(event, target != nullptr,
                                                    padTransitions)) {
                            if(target)
                                for(const auto &transition : padTransitions)
                                    target->HandleGamepadKey(transition);
                            padTransitions.clear();
                        } else {
                            for(const auto &window : windows)
                                window->Handle(event);
                        }
                    }
                } while(SDL_PollEvent(&event));
            }
            auto *keyboardFocus = SDL_GetKeyboardFocus();
            const bool active =
                std::any_of(windows.begin(), windows.end(),
                            [keyboardFocus](const auto &window) {
                                return window->Owns(keyboardFocus);
                            });
#ifdef __ANDROID__
            const bool foreground = active && !session->background;
#else
            const bool foreground = active;
#endif
            if(session->foreground != foreground) {
                session->foreground = foreground;
                if(foreground)
                    TVPOnHostForeground();
                else
                    TVPOnHostBackground();
            }
#ifdef __ANDROID__
            const auto frame = SDL_GetTicksNS();
            if(session->background || frame < session->nextFrame)
                return;
            session->nextFrame =
                std::max(frame, session->nextFrame) + session->frameInterval;
#endif
            TVPUpdateEngineFrame();
            const auto now = SDL_GetTicksNS();
            const auto snapshot = windows;
            for(const auto &window : snapshot) {
                window->TickVideos(now);
                window->Present();
            }
            windows.erase(std::remove_if(windows.begin(), windows.end(),
                                         [](const auto &window) {
                                             return !window->Live();
                                         }),
                          windows.end());
        }
    } // namespace

    void Capture(const std::string &path,
                 std::function<void(bool)> completion) {
        bool saved = false;
        if(session) {
            auto *target = focusedWindow;
            if(!target && !session->windows.empty())
                target = session->windows.front().get();
            if(target)
                saved = target->Capture(path);
        }
        completion(saved);
    }

    int DrawOnce(int interval) {
        static krkr::event::ModalFramePacer pacer;
        return pacer.DrawIfDue(
            SDL_GetTicks(), interval,
            [] {
                if(session) {
                    const auto windows = session->windows;
                    for(const auto &window : windows)
                        window->Present();
                }
            },
            [] {});
    }

    bool Active() { return session != nullptr; }

    iWindowLayer *CreateGameWindow(tTJSNI_Window *owner) {
        auto window = std::make_shared<Window>(owner, session->gamepads);
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

    std::unique_ptr<video::Presentation>
    CreateVideoPresentation(iWindowLayer &window,
                            video::FrameCallback onFrame) {
        return static_cast<Window &>(window).CreateVideoPresentation(
            std::move(onFrame));
    }

    void ReportPreferenceWriteFailure() {
        auto *locale = LocaleConfigManager::GetInstance();
#ifdef __ANDROID__
        ShowMessage(locale->GetText("cannot_create_preference"),
                    locale->GetText("readonly_storage"), {});
#else
        if(!SDL_ShowSimpleMessageBox(
               SDL_MESSAGEBOX_ERROR,
               locale->GetText("readonly_storage").c_str(),
               locale->GetText("cannot_create_preference").c_str(), nullptr)) {
            std::fprintf(stderr, "krkr2: cannot save preferences (%s)\n",
                         SDL_GetError());
        }
#endif
    }

    int Run(const std::string &game) {
        int result = 1;
        try {
            Session host;
            // The desktop entry installs native providers before CLI archive
            // validation. Keep that same scope for configuration and cleanup.
            if(!krkr::assets::CurrentReader())
                throw std::logic_error(
                    "SDL3 host resources are not initialized");
            krkr::config::RequirePreferenceStorage();
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
#ifdef __ANDROID__
                auto &settings = *IndividualConfigManager::GetInstance();
                const auto fps = settings.GetValue<int>("fps_limit", 60);
                host.frameInterval = SDL_NS_PER_SECOND / std::max(1, fps);
                if(settings.GetValue<bool>(
                       "keep_screen_alive",
                       TVPPreferenceDefaults::value_keep_screen_alive))
                    SDL_DisableScreenSaver();
                else
                    SDL_EnableScreenSaver();
#endif
                Application->StartApplication(ttstr(game));
#if KRKR2_ENABLE_UI_TEST_DRIVER
                const auto contractRequest =
                    [&](const tjs_char *option,
                        const char *filename) -> std::filesystem::path {
                    tTJSVariant enabled;
                    if(!TVPGetCommandLine(option, &enabled) ||
                       ttstr(enabled) != TJS_W("yes"))
                        return {};
                    const auto root = std::filesystem::u8path(game);
                    if(!std::filesystem::is_regular_file(
                           root / "interaction-fixture.json"))
                        throw std::runtime_error(
                            "SDL input driver requires an isolated fixture");
                    return root / filename;
                };
                host.gamepadContractRequest = contractRequest(
                    TJS_W("-sdl-gamepad-contract"), "sdl-pad-input.txt");
                host.touchContractRequest = contractRequest(
                    TJS_W("-sdl-touch-contract"), "sdl-touch-input.txt");
#endif
                while(!host.windows.empty() && !host.ending)
                    PumpFrame();
                if(Application->IsTarminate())
                    TVPUpdateEngineFrame();
                if(host.startedWindow)
                    result = 0;
                else if(!host.ending)
                    std::fprintf(
                        stderr, "krkr2: SDL3 startup created no game window\n");
            } catch(const std::exception &error) {
                std::fprintf(stderr, "krkr2: %s\n", error.what());
#ifdef __ANDROID__
                ShowMessage(error.what(), "KrKr2", {});
#endif
            }
            // Invalidate engine windows before destroying their host forms.
            CloseNativeDialogs();
            TVPSystemUninit();
        } catch(const std::exception &error) {
            std::fprintf(stderr, "krkr2: %s\n", error.what());
#ifdef __ANDROID__
            ShowMessage(error.what(), "KrKr2", {});
#endif
            result = 1;
        }
        session = nullptr;
        focusedWindow = modalWindow = nullptr;
        return result;
    }
} // namespace krkr::sdl
