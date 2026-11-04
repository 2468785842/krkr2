// Deliberately separate shell experiment; this does not implement iWindowLayer.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "InputState.h"
#include "InputModifiers.h"
#include "SdlPointerInput.h"
#include "PointerSelfTest.h"
#include "tjs.h"
#include "tjsError.h"
#include "TVPInitLog.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace TJS;
namespace fs = std::filesystem;

// This small TJS host has no engine locale manager.
TJS::ttstr TVPGetMessageByLocale(const std::string &key) { return ttstr(key); }

namespace {
    void check(bool success) {
        if(!success)
            throw std::runtime_error(SDL_GetError());
    }

    struct Script {
        tTJS *engine = new tTJS;
        ~Script() { engine->Release(); }
        tTJSVariant evaluate(const std::string &expression) {
            tTJSVariant result;
            engine->EvalExpression(ttstr(expression), &result);
            return result;
        }
        int integer(const char *name) {
            return static_cast<tjs_int>(evaluate(name));
        }
        int optionalInteger(const tjs_char *name, int fallback) {
            tTJSVariant value;
            auto *global = engine->GetGlobalNoAddRef();
            const auto result =
                global->PropGet(0, name, nullptr, &value, global);
            if(result == TJS_E_MEMBERNOTFOUND)
                return fallback;
            if(TJS_FAILED(result))
                throw std::runtime_error("Cannot read optional canvas size");
            return static_cast<tjs_int>(value);
        }
        void execute(const std::string &code) {
            engine->ExecScript(ttstr(code));
        }
    };

    int virtualKey(const SDL_KeyboardEvent &event) {
        // Virtual keys describe keys, not composed/shifted text. In particular
        // Ctrl/Shift+A must still reach the engine as VK_A.
        const auto key =
            SDL_GetKeyFromScancode(event.scancode, SDL_KMOD_NONE, true);
        if(key >= SDLK_A && key <= SDLK_Z)
            return 'A' + (key - SDLK_A);
        if(key >= SDLK_0 && key <= SDLK_9)
            return '0' + (key - SDLK_0);
        switch(key) {
            case SDLK_LSHIFT:
            case SDLK_RSHIFT:
                return krkr::input::key::shift;
            case SDLK_LCTRL:
            case SDLK_RCTRL:
                return krkr::input::key::control;
            case SDLK_LALT:
            case SDLK_RALT:
                return krkr::input::key::alt;
            case SDLK_ESCAPE:
                return 0x1b;
            case SDLK_RETURN:
                return 0x0d;
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

    void setColor(SDL_Renderer *renderer, unsigned color) {
        check(SDL_SetRenderDrawColor(renderer, (color >> 16) & 255,
                                     (color >> 8) & 255, color & 255, 255));
    }

    void capture(SDL_Renderer *renderer, const fs::path &path) {
        std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface(
            SDL_RenderReadPixels(renderer, nullptr), SDL_DestroySurface);
        if(!surface)
            throw std::runtime_error(SDL_GetError());
        check(SDL_SaveBMP(surface.get(), path.u8string().c_str()));
    }

    void push(SDL_WindowID window, unsigned stage) {
        SDL_Event event{};
        if(stage == 0) {
            const SDL_Scancode modifiers[] = { SDL_SCANCODE_LSHIFT,
                                               SDL_SCANCODE_LCTRL,
                                               SDL_SCANCODE_LALT };
            for(auto scancode : modifiers) {
                event = {};
                event.type = SDL_EVENT_KEY_DOWN;
                event.key.windowID = window;
                event.key.scancode = scancode;
                event.key.key =
                    SDL_GetKeyFromScancode(scancode, SDL_KMOD_NONE, true);
                event.key.down = true;
                check(SDL_PushEvent(&event));
            }
            event = {};
        }
        if(stage == 0 || stage == 3 || stage == 4) {
            event.type = stage == 4 ? SDL_EVENT_KEY_UP : SDL_EVENT_KEY_DOWN;
            event.key.windowID = window;
            event.key.key = SDLK_A;
            event.key.scancode = SDL_SCANCODE_A;
            event.key.down = stage != 4;
        } else if(stage == 1 || stage == 2) {
            if(stage == 2) {
                // A repeat and a late physical release while unfocused must
                // not re-arm the key or send a second script key-up.
                event.type = SDL_EVENT_KEY_DOWN;
                event.key.windowID = window;
                event.key.key = SDLK_A;
                event.key.scancode = SDL_SCANCODE_A;
                event.key.down = true;
                event.key.repeat = true;
                check(SDL_PushEvent(&event));
                event.type = SDL_EVENT_KEY_UP;
                event.key.down = false;
                event.key.repeat = false;
                check(SDL_PushEvent(&event));
                event = {};
            }
            event.type = stage == 1 ? SDL_EVENT_WINDOW_FOCUS_LOST
                                    : SDL_EVENT_WINDOW_FOCUS_GAINED;
            event.window.windowID = window;
        } else {
            event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
            event.window.windowID = window;
        }
        check(SDL_PushEvent(&event));
        if(stage == 1) {
            check(SDL_PushEvent(&event)); // Duplicate loss is idempotent.
            event.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
            event.window.windowID = 0;
            check(SDL_PushEvent(&event)); // Foreign focus cannot reactivate us.
        } else if(stage == 2) {
            check(SDL_PushEvent(&event)); // Duplicate gain is idempotent.
            event = {};
            event.type = SDL_EVENT_KEY_UP;
            event.key.windowID = window;
            event.key.key = SDLK_A;
            event.key.scancode = SDL_SCANCODE_A;
            check(SDL_PushEvent(&event)); // Late release after recovery.
        }
        if(stage == 0) {
            // An unrelated window must not release our held A or close us.
            event.key.windowID = 0;
            event.type = SDL_EVENT_KEY_UP;
            event.key.down = false;
            check(SDL_PushEvent(&event));
            event = {};
            event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
            check(SDL_PushEvent(&event));
        }
    }

    std::string describePresentation(SDL_Window *window, SDL_Renderer *renderer,
                                     int logicalWidth, int logicalHeight) {
        int width, height, pixelWidth, pixelHeight;
        check(SDL_GetWindowSize(window, &width, &height));
        check(SDL_GetWindowSizeInPixels(window, &pixelWidth, &pixelHeight));
        SDL_FRect rectangle;
        check(SDL_GetRenderLogicalPresentationRect(renderer, &rectangle));
        std::ostringstream description;
        description.imbue(std::locale::classic());
        description << std::setprecision(
                           std::numeric_limits<float>::max_digits10)
                    << "\"actual_width\":" << width
                    << ",\"actual_height\":" << height
                    << ",\"pixel_width\":" << pixelWidth
                    << ",\"pixel_height\":" << pixelHeight
                    << ",\"logical_width\":" << logicalWidth
                    << ",\"logical_height\":" << logicalHeight
                    << ",\"presentation\":[" << rectangle.x << ','
                    << rectangle.y << ',' << rectangle.w << ',' << rectangle.h
                    << ']';
        return description.str();
    }

    int run(const fs::path &source, const fs::path &output, bool selfTest,
            bool pointerTest) {
        std::ifstream input(source, std::ios::binary);
        if(!input)
            throw std::runtime_error("Cannot read script");
        std::ostringstream code;
        code << input.rdbuf();
        if(!fs::create_directory(output))
            throw std::runtime_error("Output directory must be new");
        Script script;
        script.execute(code.str());
        const int width = script.integer("windowWidth");
        const int height = script.integer("windowHeight");
        const int canvasWidth =
            script.optionalInteger(TJS_W("canvasWidth"), width);
        const int canvasHeight =
            script.optionalInteger(TJS_W("canvasHeight"), height);
        if(width < 1 || height < 1 || width > 4096 || height > 4096)
            throw std::runtime_error("Script window size must be 1..4096");
        if(canvasWidth < 1 || canvasHeight < 1 || canvasWidth > 4096 ||
           canvasHeight > 4096)
            throw std::runtime_error("Script canvas size must be 1..4096");
        check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));
        struct Quit {
            ~Quit() { SDL_Quit(); }
        } quit;
        // SDL otherwise adds QUIT after the last window's close request,
        // asking the script twice and defeating its first veto. Genuine
        // application QUIT events (menu/signal) still use the query below.
        check(SDL_SetHintWithPriority(SDL_HINT_QUIT_ON_LAST_WINDOW_CLOSE, "0",
                                      SDL_HINT_OVERRIDE));
        SDL_Window *rawWindow = nullptr;
        SDL_Renderer *rawRenderer = nullptr;
        const auto caption =
            ttstr(script.evaluate("windowCaption")).AsStdString();
        // Software rendering is intentional: this experiment does not migrate
        // GL.
        check(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software"));
        check(SDL_CreateWindowAndRenderer(caption.c_str(), width, height,
                                          SDL_WINDOW_RESIZABLE |
                                              SDL_WINDOW_HIGH_PIXEL_DENSITY,
                                          &rawWindow, &rawRenderer));
        std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window(
            rawWindow, SDL_DestroyWindow);
        std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
            rawRenderer, SDL_DestroyRenderer);
        const char *driver = SDL_GetRendererName(renderer.get());
        if(!driver || std::string(driver) != "software")
            throw std::runtime_error(
                "The requested software renderer was not created");
        int actualWidth, actualHeight;
        check(SDL_GetWindowSize(window.get(), &actualWidth, &actualHeight));
        if(actualWidth != width || actualHeight != height ||
           caption != SDL_GetWindowTitle(window.get()))
            throw std::runtime_error(
                "Actual window differs from script configuration");
        check(SDL_SetRenderLogicalPresentation(
            renderer.get(), canvasWidth, canvasHeight,
            SDL_LOGICAL_PRESENTATION_LETTERBOX));
        std::ofstream events(output / "events.txt");
        events << std::unitbuf; // Allow an external OS-input probe to await
                                // callbacks.
        const auto windowID = SDL_GetWindowID(window.get());
        auto &state = krkr::input::sharedState();
        krkr::sdlprototype::PointerInput pointer(renderer.get(), windowID,
                                                 state);
        std::ofstream pointerEvents(output / "pointer-events.jsonl");
        pointerEvents.imbue(std::locale::classic());
        pointerEvents << std::boolalpha
                      << std::setprecision(
                             std::numeric_limits<float>::max_digits10);
        std::ofstream presentationEvents(output / "presentation-events.jsonl");
        std::string lastPresentation;
        const auto pointerCallback =
            [&](const krkr::sdlprototype::PointerEvent &event) {
                std::ostringstream call;
                call.imbue(std::locale::classic());
                call << std::boolalpha
                     << std::setprecision(
                            std::numeric_limits<float>::max_digits10)
                     << "onPointer(\"" << event.kind << "\","
                     << event.position.x << ',' << event.position.y << ','
                     << event.button << ',' << event.shift << ','
                     << event.current << ',' << event.pending << ','
                     << event.wheelX << ',' << event.wheelY << ','
                     << event.synthetic << ");";
                script.execute(call.str());
                pointerEvents << "{\"kind\":\"" << event.kind
                              << "\",\"x\":" << event.position.x
                              << ",\"y\":" << event.position.y
                              << ",\"button\":" << event.button
                              << ",\"shift\":" << event.shift
                              << ",\"current\":" << event.current
                              << ",\"pending\":" << event.pending
                              << ",\"synthetic\":" << event.synthetic
                              << ",\"wheel_x\":" << event.wheelX
                              << ",\"wheel_y\":" << event.wheelY << "}\n"
                              << std::flush;
            };
        bool running = true;
        // Self-test starts with virtual focus, just like its injected keys;
        // interactive use starts with the actual window's focus state.
        bool focused = selfTest || pointerTest ||
            (SDL_GetWindowFlags(window.get()) & SDL_WINDOW_INPUT_FOCUS) != 0;
        unsigned stage = 0, frames = 0, keyDowns = 0, keyUps = 0, vetoes = 0;
        unsigned focusLosses = 0, focusGains = 0, ignoredKeys = 0;
        unsigned syntheticReleases = 0, foreignEvents = 0;
        const auto keyCallback = [&](int key, bool down) {
            const auto shift = krkr::input::legacyShiftFlags(state);
            const bool current = state.query(key, true);
            const bool pending = state.query(key, false);
            script.execute("onKey(" + std::to_string(key) + "," +
                           (down ? "true" : "false") + "," +
                           (current ? "true" : "false") + "," +
                           (pending ? "true" : "false") + "," +
                           std::to_string(shift) + ");");
            events << "key:" << key << ':' << down << ':' << current << ':'
                   << pending << ':' << shift << '\n';
        };
        const auto started = SDL_GetTicks();
        while(running) {
            if((selfTest || pointerTest) && SDL_GetTicks() - started > 10000)
                throw std::runtime_error("Self-test deadline expired");
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                if(krkr::sdlprototype::PointerInput::accepts(
                       static_cast<SDL_EventType>(event.type))) {
                    if(auto converted = pointer.handle(event, focused))
                        pointerCallback(*converted);
                } else if(event.type == SDL_EVENT_KEY_DOWN ||
                          event.type == SDL_EVENT_KEY_UP) {
                    if(event.key.windowID != windowID) {
                        ++foreignEvents;
                        continue;
                    }
                    const auto key = virtualKey(event.key);
                    if(!key) {
                        events << "unsupported-key:" << event.key.key << '\n';
                        continue;
                    }
                    const bool down = event.type == SDL_EVENT_KEY_DOWN;
                    if(!focused) {
                        ++ignoredKeys;
                        events << "unfocused-key:" << key << ':' << down
                               << '\n';
                        continue;
                    }
                    if(down) {
                        state.press(key);
                        ++keyDowns;
                    } else {
                        if(!state.release(key))
                            continue;
                        ++keyUps;
                    }
                    keyCallback(key, down);
                } else if(event.type == SDL_EVENT_WINDOW_FOCUS_LOST ||
                          event.type == SDL_EVENT_WINDOW_FOCUS_GAINED) {
                    if(event.window.windowID != windowID) {
                        ++foreignEvents;
                        continue;
                    }
                    const bool gained =
                        event.type == SDL_EVENT_WINDOW_FOCUS_GAINED;
                    if(focused == gained)
                        continue;
                    focused = gained;
                    if(!focused) {
                        ++focusLosses;
                        for(auto key : state.releaseAll()) {
                            ++syntheticReleases;
                            if(auto released = pointer.focusRelease(key))
                                pointerCallback(*released);
                            else {
                                ++keyUps;
                                keyCallback(key, false);
                            }
                        }
                    } else {
                        ++focusGains;
                    }
                    script.execute(focused ? "onFocus(true);"
                                           : "onFocus(false);");
                    events << "focus:" << focused << '\n';
                } else if(event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
                          event.type == SDL_EVENT_QUIT) {
                    if(event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                       event.window.windowID != windowID) {
                        ++foreignEvents;
                        continue;
                    }
                    running = static_cast<tjs_int>(
                                  script.evaluate("onCloseQuery()")) == 0;
                    events << "close-accepted:" << !running << '\n';
                    if(running)
                        ++vetoes;
                    else
                        break;
                }
            }
            if(!running)
                break;
            setColor(renderer.get(), script.integer("backgroundColor"));
            check(SDL_RenderClear(renderer.get()));
            setColor(renderer.get(), script.integer("rectangleColor"));
            SDL_FRect rectangle{ float(script.integer("rectangleX")),
                                 float(script.integer("rectangleY")),
                                 float(script.integer("rectangleWidth")),
                                 float(script.integer("rectangleHeight")) };
            check(SDL_RenderFillRect(renderer.get(), &rectangle));
            if(selfTest) {
                const char *names[] = { "initial.bmp",   "pressed.bmp",
                                        "unfocused.bmp", "restored.bmp",
                                        "repressed.bmp", "released.bmp",
                                        "veto.bmp" };
                if(stage < 7)
                    capture(renderer.get(), output / names[stage]);
            } else if(pointerTest &&
                      stage < krkr::sdlprototype::pointerTestFrames) {
                capture(renderer.get(),
                        output / krkr::sdlprototype::pointerCaptureName(stage));
            }
            check(SDL_RenderPresent(renderer.get()));
            // A completed frame acknowledges the new viewport to OS probes.
            const auto presentation = describePresentation(
                window.get(), renderer.get(), canvasWidth, canvasHeight);
            if(presentation != lastPresentation) {
                presentationEvents << '{' << presentation << "}\n"
                                   << std::flush;
                lastPresentation = presentation;
            }
            ++frames;
            if(selfTest && stage < 7)
                push(windowID, stage++);
            else if(pointerTest &&
                    stage < krkr::sdlprototype::pointerTestFrames)
                krkr::sdlprototype::pushPointerTest(windowID, stage++);
            SDL_Delay(16);
        }
        if(selfTest &&
           (keyDowns != 5 || keyUps != 5 || vetoes != 1 || focusLosses != 1 ||
            focusGains != 1 || ignoredKeys != 2 || syntheticReleases != 4 ||
            foreignEvents != 3 || script.integer("keyDownCount") != 2 ||
            script.integer("keyUpCount") != 2 ||
            script.integer("modifierDownCount") != 3 ||
            script.integer("modifierUpCount") != 3 ||
            script.integer("focusLossCount") != 1 ||
            script.integer("focusGainCount") != 1 ||
            script.integer("closeCount") != 2))
            throw std::runtime_error("Self-test event or script counts differ");
        const auto &pointerStats = pointer.stats();
        if(pointerTest &&
           (frames != krkr::sdlprototype::pointerTestFrames ||
            pointerStats.downs != 2 || pointerStats.ups != 2 ||
            pointerStats.wheels != 1 || pointerStats.syntheticUps != 1 ||
            pointerStats.moves < 3 || pointerStats.unfocused < 2 ||
            pointerStats.foreign < 1 || pointerStats.redundantUps < 1 ||
            focusLosses != 1 || focusGains != 1 || vetoes != 1 ||
            script.integer("pointerDownCount") != 2 ||
            script.integer("pointerUpCount") != 2 ||
            script.integer("wheelCount") != 1 ||
            script.integer("focusLossCount") != 1 ||
            script.integer("focusGainCount") != 1 ||
            script.integer("closeCount") != 2))
            throw std::runtime_error("Pointer self-test counts differ");
        std::ofstream report(output / "report.json");
        report << "{\"completed\":true,\"renderer\":\"software\",\"self_test\":"
               << (selfTest ? "true" : "false") << ",\"video_driver\":\""
               << SDL_GetCurrentVideoDriver() << '"'
               << ",\"sdl_version\":" << SDL_GetVersion()
               << ",\"frames\":" << frames << ",\"key_downs\":" << keyDowns
               << ",\"key_ups\":" << keyUps << ",\"close_vetoes\":" << vetoes
               << ",\"focus_losses\":" << focusLosses
               << ",\"focus_gains\":" << focusGains
               << ",\"ignored_keys\":" << ignoredKeys
               << ",\"synthetic_releases\":" << syntheticReleases
               << ",\"foreign_events\":" << foreignEvents
               << ",\"pointer_test\":" << (pointerTest ? "true" : "false")
               << ",\"pointer_moves\":" << pointerStats.moves
               << ",\"pointer_downs\":" << pointerStats.downs
               << ",\"pointer_ups\":" << pointerStats.ups
               << ",\"pointer_wheels\":" << pointerStats.wheels
               << ",\"pointer_synthetic_ups\":" << pointerStats.syntheticUps
               << ",\"pointer_foreign\":" << pointerStats.foreign
               << ",\"pointer_unfocused\":" << pointerStats.unfocused
               << ",\"pointer_redundant_ups\":" << pointerStats.redundantUps
               << ','
               << describePresentation(window.get(), renderer.get(),
                                       canvasWidth, canvasHeight)
               << ",\"width\":" << width << ",\"height\":" << height << "}\n";
        return 0;
    }
} // namespace

int main(int argc, char **argv) {
    try {
        TVPInitLogging();
        if(argc < 3 || argc > 4 ||
           (argc == 4 && std::string(argv[3]) != "--self-test" &&
            std::string(argv[3]) != "--pointer-test"))
            throw std::runtime_error(
                "Usage: krkr2_sdl3_prototype script.tjs "
                "NEW_OUTPUT_DIR [--self-test|--pointer-test]");
        return run(fs::u8path(argv[1]), fs::u8path(argv[2]),
                   argc == 4 && std::string(argv[3]) == "--self-test",
                   argc == 4 && std::string(argv[3]) == "--pointer-test");
    } catch(const eTJS &error) {
        std::cerr << "TJS: " << error.getMessage().AsStdString() << '\n';
    } catch(const std::exception &error) {
        std::cerr << error.what() << '\n';
    }
    return 1;
}
