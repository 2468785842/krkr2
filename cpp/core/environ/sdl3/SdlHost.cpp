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
#include "InputModifiers.h"
#include "InputState.h"
#include "MsgIntf.h"
#include "RenderManager.h"
#include "SysInitIntf.h"
#include "TVPLog.h"
#include "WindowImpl.h"
#include "WindowLayer.h"

#include <algorithm>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>

extern std::thread::id TVPMainThreadID;
std::string TVPGetCurrentLanguage();

namespace krkr::sdl {
    namespace {
        void check(bool success) {
            if(!success)
                throw std::runtime_error(std::string("SDL3: ") +
                                         SDL_GetError());
        }

        struct VideoSubsystem {
            VideoSubsystem() { check(SDL_InitSubSystem(SDL_INIT_VIDEO)); }
            ~VideoSubsystem() { SDL_QuitSubSystem(SDL_INIT_VIDEO); }
        };

        unsigned virtualKey(const SDL_KeyboardEvent &event) {
            const auto key =
                SDL_GetKeyFromScancode(event.scancode, SDL_KMOD_NONE, true);
            if(key >= SDLK_A && key <= SDLK_Z)
                return 'A' + (key - SDLK_A);
            if(key >= SDLK_0 && key <= SDLK_9)
                return '0' + (key - SDLK_0);
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
                default:
                    return 0;
            }
        }

        class Window final : public iWindowLayer {
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
            bool visible = false, focused = false;
            bool dirty = true, closePending = false;
            tTVPImeMode defaultIme = imDisable;

        public:
            explicit Window(tTJSNI_Window *owner) : native(owner) {
                window.reset(
                    SDL_CreateWindow(caption.c_str(), width, height,
                                     SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN));
                check(window != nullptr);
                renderer.reset(SDL_CreateRenderer(window.get(), "software"));
                check(renderer != nullptr);
                SetPaintBoxSize(layerWidth, layerHeight);
                G_CoreLog.info("SDL3 video={}, renderer={}",
                               SDL_GetCurrentVideoDriver(),
                               SDL_GetRendererName(renderer.get()));
            }

            bool Live() const { return native != nullptr; }
            void SetPaintBoxSize(tjs_int w, tjs_int h) override {
                if(w <= 0 || h <= 0)
                    return;
                check(SDL_SetRenderLogicalPresentation(
                    renderer.get(), w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX));
                layerWidth = w;
                layerHeight = h;
                dirty = true;
            }
            bool GetFormEnabled() override { return Live() && visible; }
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
            }
            void SetHintText(const ttstr &) override {}
            void SetAttentionPoint(tjs_int, tjs_int,
                                   const tTVPFont *) override {}
            void ZoomRectangle(tjs_int &, tjs_int &, tjs_int &,
                               tjs_int &) override {}
            void BringToFront() override {
                check(SDL_RaiseWindow(window.get()));
            }
            void ShowWindowAsModal() override {
                TVPThrowExceptionMessage(
                    TJS_W("SDL3 does not support modal game windows"));
            }
            bool GetVisible() override { return visible; }
            void SetVisible(bool value) override {
                if(visible == value)
                    return;
                check(value ? SDL_ShowWindow(window.get())
                            : SDL_HideWindow(window.get()));
                visible = value;
                if(visible) {
                    RequestRepaint();
                } else {
                    ReleaseInput();
                }
                dirty = true;
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
            void SetZoom(tjs_int numer, tjs_int denom) override {
                if(numer <= 0 || numer != denom)
                    TVPThrowExceptionMessage(
                        TJS_W("SDL3 supports only 1:1 script zoom"));
                ZoomNumer = numer;
                ZoomDenom = denom;
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
                    const auto *sample = static_cast<const std::uint8_t *>(
                                             frame->Row(frame->height / 4)) +
                        4u * (frame->width / 4);
                    G_CoreLog.debug("SDL3 upload {}x{}; pitch={}; "
                                    "sample={:02x}{:02x}{:02x}{:02x}",
                                    frame->width, frame->height, frame->pitch,
                                    sample[0], sample[1], sample[2], sample[3]);
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
                native = nullptr;
                input::sharedState().releaseAll();
                texture.reset();
                renderer.reset();
                window.reset();
            }
            bool GetWindowActive() override {
                return GetFormEnabled() && focused;
            }
            void Close() override {
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
            void SetDefaultImeMode(tTVPImeMode mode, bool) override {
                defaultIme = mode;
            }
            void SetImeMode(tTVPImeMode) override {}
            void ResetImeMode() override {}
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
            void ResetMouseVelocity() override {}
            void ResetTouchVelocity(tjs_int) override {}
            bool GetMouseVelocity(float &, float &, float &) const override {
                return false;
            }
            void TickBeat() override {}

            void Present() {
                if(!Live() || !visible || !dirty)
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
                    if(!sampledPresent) {
                        const SDL_Rect point{ layerWidth / 4, layerHeight / 4,
                                              1, 1 };
                        auto *surface =
                            SDL_RenderReadPixels(renderer.get(), &point);
                        Uint8 r = 0, g = 0, b = 0, a = 0;
                        const bool sampled = surface &&
                            SDL_ReadSurfacePixel(surface, 0, 0, &r, &g, &b, &a);
                        SDL_DestroySurface(surface);
                        G_CoreLog.debug("SDL3 rendered sample ({}): "
                                        "{:02x}{:02x}{:02x}{:02x}",
                                        sampled, r, g, b, a);
                        sampledPresent = true;
                    }
                }
                check(SDL_RenderPresent(renderer.get()));
                dirty = false;
            }

            void Handle(const SDL_Event &event) {
                if(!Live())
                    return;
                const auto id = SDL_GetWindowID(window.get());
                switch(event.type) {
                    case SDL_EVENT_QUIT:
                        Close();
                        break;
                    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                        if(event.window.windowID == id)
                            Close();
                        break;
                    case SDL_EVENT_WINDOW_FOCUS_GAINED:
                    case SDL_EVENT_WINDOW_FOCUS_LOST:
                        if(event.window.windowID == id) {
                            focused =
                                event.type == SDL_EVENT_WINDOW_FOCUS_GAINED;
                            if(!focused)
                                ReleaseInput();
                        }
                        break;
                    case SDL_EVENT_WINDOW_RESIZED:
                        if(event.window.windowID == id) {
                            width = event.window.data1;
                            height = event.window.data2;
                            dirty = true;
                            TVPPostInputEvent(
                                new tTVPOnResizeInputEvent(native));
                        }
                        break;
                    case SDL_EVENT_WINDOW_EXPOSED:
                    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                        if(event.window.windowID == id) {
                            if(!texture)
                                RequestRepaint();
                            dirty = true;
                        }
                        break;
                    case SDL_EVENT_KEY_DOWN:
                    case SDL_EVENT_KEY_UP:
                        if(event.key.windowID == id && GetWindowActive())
                            HandleKey(event.key);
                        break;
                    case SDL_EVENT_MOUSE_MOTION:
                        if(event.motion.windowID == id && GetWindowActive()) {
                            SetPointer(event.motion.x, event.motion.y);
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
                }
            }

        private:
            bool sampledPresent = false;

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
            bool SetPointer(float x, float y) {
                float logicalX, logicalY;
                check(SDL_RenderCoordinatesFromWindow(renderer.get(), x, y,
                                                      &logicalX, &logicalY));
                mouseX = static_cast<int>(logicalX);
                mouseY = static_cast<int>(logicalY);
                return logicalX >= 0 && logicalY >= 0 &&
                    logicalX < layerWidth && logicalY < layerHeight;
            }
            void HandleKey(const SDL_KeyboardEvent &event) {
                const auto key = virtualKey(event);
                if(!key)
                    return;
                auto &state = input::sharedState();
                if(event.down) {
                    state.press(key);
                    InternalKeyDown(
                        key, ShiftFlags() | (event.repeat ? ssRepeat : 0));
                } else if(state.release(key)) {
                    OnKeyUp(key, ShiftFlags());
                }
            }
            void HandleButton(const SDL_MouseButtonEvent &event) {
                if(event.button != SDL_BUTTON_LEFT)
                    return;
                const bool inside = SetPointer(event.x, event.y);
                auto &state = input::sharedState();
                if(event.down) {
                    if(!inside)
                        return;
                    state.press(input::key::leftMouse);
                    TVPPostInputEvent(new tTVPOnMouseDownInputEvent(
                        native, mouseX, mouseY, mbLeft, ShiftFlags()));
                } else if(state.release(input::key::leftMouse)) {
                    TVPPostInputEvent(new tTVPOnMouseUpInputEvent(
                        native, mouseX, mouseY, mbLeft, ShiftFlags()));
                }
            }
            void ReleaseInput() {
                for(auto key : input::sharedState().releaseAll()) {
                    if(key == input::key::leftMouse) {
                        TVPPostInputEvent(new tTVPOnMouseUpInputEvent(
                            native, mouseX, mouseY, mbLeft, ShiftFlags(),
                            true));
                    } else {
                        OnKeyUp(key, ShiftFlags());
                    }
                }
            }
        };

        struct Session {
            VideoSubsystem video;
            std::unique_ptr<Window> window;
        };

        Session *session = nullptr;
    } // namespace

    bool Active() { return session != nullptr; }

    iWindowLayer *CreateWindow(tTJSNI_Window *owner) {
        if(session->window && session->window->Live())
            TVPThrowExceptionMessage(TJS_W("SDL3 supports one game window"));
        session->window = std::make_unique<Window>(owner);
        return session->window.get();
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
                while(host.window && host.window->Live()) {
                    SDL_Event event;
                    if(SDL_WaitEventTimeout(&event, 16)) {
                        do {
                            host.window->Handle(event);
                        } while(SDL_PollEvent(&event));
                    }
                    TVPUpdateEngineFrame();
                    host.window->Present();
                }
                if(Application->IsTarminate())
                    TVPUpdateEngineFrame();
                if(host.window)
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
        return result;
    }
} // namespace krkr::sdl
