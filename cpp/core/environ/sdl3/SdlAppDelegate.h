#pragma once

//---------------------------------------------------------------------------
// SDL3 shell application: owns the window, the GL context and the event pump,
// replacing environ/cocos2d/AppDelegate + platforms/{linux,windows,apple}/main
// for the SDL3 desktop build.
//
// Window  : SDL_CreateWindow(SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY)
//           + SDL_GL context, RGBA8/depth24/stencil8 (AppDelegate.cpp:146-148)
// Loop    : SDL_PollEvent dispatch -> engine hooks -> frame callback ->
//           SDL_GL_SwapWindow (prototype tools/sdl3-prototype/main.cpp:331-448)
// No cocos headers are referenced here.
//---------------------------------------------------------------------------

#include <SDL3/SDL.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>

// Forward declaration of the engine window-native base (WindowIntf.h), so
// this header does not need to include the engine window interface chain.
class tTJSNI_BaseWindow;

namespace krkr::sdl3 {

class SdlWindowLayer;

// IME bridge for SDL_EVENT_TEXT_INPUT / SDL_EVENT_TEXT_EDITING.
// The engine-side adapter -- insertText -> decodeTextInput -> per-UTF-16-unit
// Window.OnKeyPress, deleteBackward -> VK_BACK down/up -- exists on the cocos
// host as MainScene's IMEDelegate (MainScene.cpp:1278, :1291). Wiring that
// adapter to this interface is B2-2; the shell only defines and calls it.
class SdlIMEDelegate {
public:
    virtual ~SdlIMEDelegate() = default;
    // Committed text (UTF-8).
    virtual void insertText(const char *text, std::size_t len) = 0;
    // Backspace while composing (non-Win32 hosts post VK_BACK down/up).
    virtual void deleteBackward() = 0;
    // Active preedit string and the selection span inside it. Desktop builds
    // never had preedit before; reaching parity needs OnKeyPress plumbing
    // beyond this interface (B2-2).
    virtual void setPreedit(const char *text, int start, int length) = 0;
};

class SdlAppDelegate {
public:
    struct Config {
        // Matches designResolutionSize (AppDelegate.cpp:24) and the window
        // GLViewImpl::createWithRect("krkr2", 960x640) (AppDelegate.cpp:58-61).
        std::string title = "krkr2";
        int width = 960;
        int height = 640;
        bool fullscreen = false;
    };

    SdlAppDelegate();
    explicit SdlAppDelegate(const Config &config);
    ~SdlAppDelegate();

    SdlAppDelegate(const SdlAppDelegate &) = delete;
    SdlAppDelegate &operator=(const SdlAppDelegate &) = delete;

    // SDL_Init + window + GL context. Returns false on failure.
    bool init();

    // Pumps events until stopped. Each iteration: dispatch, engine frame
    // callback (the Application->Run() hook, MainScene.cpp:747-748), swap.
    void run();
    void stop();

    void setFullscreen(bool fullscreen);
    [[nodiscard]] bool isFullscreen() const;

    // --- engine hooks -----------------------------------------------------

    // Engine frame driver; install the Application->Run() equivalent here.
    using FrameCallback = std::function<void()>;
    void setFrameCallback(FrameCallback callback);

    // Veto closing. Mirrors the script's onCloseQuery: return false to keep
    // the window open. SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE=0 (prototype
    // main.cpp:237-241) keeps SDL from injecting a QUIT after the first
    // vetoed CLOSE_REQUESTED, preserving the "close asked twice" semantics.
    using CloseQueryCallback = std::function<bool()>;
    void setCloseQueryCallback(CloseQueryCallback callback);

    // GL context loss / drawable size change. The engine side must dispatch
    // EVENT_RENDERER_RECREATED (ogl:2968-2980, YUVSprite.cpp:43-57) and reset
    // TVPGLExtensionInfoInited (ogl:122). Both actions have stub bodies here
    // pending the B3 engine-side adapter; this callback is the wire point.
    using ContextLostCallback = std::function<void()>;
    void setContextLostCallback(ContextLostCallback callback);
    [[nodiscard]] ContextLostCallback getContextLostCallback() const;

    // IME: starts/stops SDL text input on the shell window. The engine-side
    // IME mode switch (SetImeMode) calls into this once B2-2 lands.
    void setTextInputEnabled(bool enabled);

    void setIMEDelegate(SdlIMEDelegate *delegate);
    [[nodiscard]] SdlIMEDelegate *getIMEDelegate() const;

    // --- DPI / size queries (replace cocos2d::Device::getDPI,
    // MainScene.cpp:540 touch threshold and :832 UI scaling) ---------------

    // SDL_GetWindowDisplayScale: content scale, 1.0 == baseline density.
    [[nodiscard]] float getDisplayScale() const;
    // Legacy DPI baseline of the scale above (96 dpi == scale 1.0).
    [[nodiscard]] float getDPI() const;
    void getWindowSize(int &width, int &height) const;
    void getWindowSizeInPixels(int &width, int &height) const;
    [[nodiscard]] SDL_WindowID getWindowID() const { return windowID; }

private:
    void pollEvents();
    void handleWindowEvent(const SDL_WindowEvent &event);
    void handleKeyboard(const SDL_KeyboardEvent &event);
    void postVirtualKey(std::uint32_t vk, bool down, bool repeat);
    void openInitialGamepads();
    void closeGamepads();
    void releaseWindow();

    // Window focus <-> host lifecycle (HostLifecycle.h). TVPOnHostForeground/
    // Background are the engine entry points (Application.cpp:749-751).
    void onHostForeground();
    void onHostBackground();
    // Focus loss: synthesize key/pointer releases first, then tell the engine.
    // Mirrors the InputCancellation semantics of the cocos host
    // (krkr::input::planInputCancellation, InputCancellation.h).
    void cancelInputOnFocusLoss();

    // Context-lost plumbing (see setContextLostCallback). Both perform their
    // named action through the B3 engine-side adapter; bodies documented in
    // the .cpp.
    void notifyContextLost();
    void dispatchRendererRecreated();
    void resetGLExtensionInfoInited();

    [[nodiscard]] bool allowClose() const;
    [[nodiscard]] tTJSNI_BaseWindow *currentNativeWindow() const;

    Config config;
    SDL_Window *window = nullptr;
    SDL_GLContext glContext = nullptr;
    SDL_WindowID windowID = 0;
    bool running = false;
    bool focused = false;
    bool hostForeground = false;
    bool ownsSDL = false;
    bool wasFullscreen = false;
    bool vsync = false;

    FrameCallback frameCallback;
    CloseQueryCallback closeQueryCallback;
    ContextLostCallback contextLostCallback;
    SdlIMEDelegate *imeDelegate = nullptr;

    std::unordered_map<SDL_JoystickID, SDL_Gamepad *> gamepads;
};

} // namespace krkr::sdl3
