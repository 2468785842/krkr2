// Deliberately separate shell experiment; this does not implement iWindowLayer.
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include "InputState.h"
#include "tjs.h"
#include "tjsError.h"
#include "TVPInitLog.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
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
        void execute(const std::string &code) {
            engine->ExecScript(ttstr(code));
        }
    };

    int virtualKey(SDL_Keycode key) {
        if(key >= SDLK_A && key <= SDLK_Z)
            return 'A' + (key - SDLK_A);
        if(key >= SDLK_0 && key <= SDLK_9)
            return '0' + (key - SDLK_0);
        switch(key) {
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
        if(stage <= 1) {
            event.type = stage == 0 ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
            event.key.windowID = window;
            event.key.key = SDLK_A;
            event.key.scancode = SDL_SCANCODE_A;
            event.key.down = stage == 0;
        } else {
            event.type = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
            event.window.windowID = window;
        }
        check(SDL_PushEvent(&event));
    }

    int run(const fs::path &source, const fs::path &output, bool selfTest) {
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
        if(width < 1 || height < 1 || width > 4096 || height > 4096)
            throw std::runtime_error("Script window size must be 1..4096");
        check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));
        struct Quit {
            ~Quit() { SDL_Quit(); }
        } quit;
        SDL_Window *rawWindow = nullptr;
        SDL_Renderer *rawRenderer = nullptr;
        const auto caption =
            ttstr(script.evaluate("windowCaption")).AsStdString();
        // Software rendering is intentional: this experiment does not migrate
        // GL.
        check(SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software"));
        check(SDL_CreateWindowAndRenderer(caption.c_str(), width, height, 0,
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
            renderer.get(), width, height, SDL_LOGICAL_PRESENTATION_LETTERBOX));
        std::ofstream events(output / "events.txt");
        const auto windowID = SDL_GetWindowID(window.get());
        auto &state = krkr::input::sharedState();
        bool running = true;
        unsigned stage = 0, frames = 0, keyDowns = 0, keyUps = 0, vetoes = 0;
        const auto started = SDL_GetTicks();
        while(running) {
            if(selfTest && SDL_GetTicks() - started > 10000)
                throw std::runtime_error("Self-test deadline expired");
            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                if(event.type == SDL_EVENT_KEY_DOWN ||
                   event.type == SDL_EVENT_KEY_UP) {
                    const auto key = virtualKey(event.key.key);
                    if(!key) {
                        events << "unsupported-key:" << event.key.key << '\n';
                        continue;
                    }
                    const bool down = event.type == SDL_EVENT_KEY_DOWN;
                    if(down) {
                        state.press(key);
                        ++keyDowns;
                    } else {
                        state.release(key);
                        ++keyUps;
                    }
                    const bool current = state.query(key, true);
                    const bool pending = state.query(key, false);
                    script.execute("onKey(" + std::to_string(key) + "," +
                                   (down ? "true" : "false") + "," +
                                   (current ? "true" : "false") + "," +
                                   (pending ? "true" : "false") + ");");
                    events << "key:" << key << ':' << down << ':' << current
                           << ':' << pending << '\n';
                } else if(event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED ||
                          event.type == SDL_EVENT_QUIT) {
                    running = static_cast<tjs_int>(
                                  script.evaluate("onCloseQuery()")) == 0;
                    events << "close-accepted:" << !running << '\n';
                    if(running)
                        ++vetoes;
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
                const char *names[] = { "initial.bmp", "pressed.bmp",
                                        "released.bmp", "veto.bmp" };
                if(stage < 4)
                    capture(renderer.get(), output / names[stage]);
            }
            check(SDL_RenderPresent(renderer.get()));
            ++frames;
            if(selfTest && stage < 4)
                push(windowID, stage++);
            SDL_Delay(16);
        }
        if(selfTest &&
           (keyDowns != 1 || keyUps != 1 || vetoes != 1 ||
            script.integer("keyDownCount") != 1 ||
            script.integer("keyUpCount") != 1 ||
            script.integer("closeCount") != 2))
            throw std::runtime_error("Self-test event or script counts differ");
        std::ofstream report(output / "report.json");
        report << "{\"completed\":true,\"renderer\":\"software\",\"self_test\":"
               << (selfTest ? "true" : "false")
               << ",\"sdl_version\":" << SDL_GetVersion()
               << ",\"frames\":" << frames << ",\"key_downs\":" << keyDowns
               << ",\"key_ups\":" << keyUps << ",\"close_vetoes\":" << vetoes
               << ",\"width\":" << width << ",\"height\":" << height << "}\n";
        return 0;
    }
} // namespace

int main(int argc, char **argv) {
    try {
        TVPInitLogging();
        if(argc < 3 || argc > 4 ||
           (argc == 4 && std::string(argv[3]) != "--self-test"))
            throw std::runtime_error("Usage: krkr2_sdl3_prototype script.tjs "
                                     "NEW_OUTPUT_DIR [--self-test]");
        return run(fs::u8path(argv[1]), fs::u8path(argv[2]), argc == 4);
    } catch(const eTJS &error) {
        std::cerr << "TJS: " << error.getMessage().AsStdString() << '\n';
    } catch(const std::exception &error) {
        std::cerr << error.what() << '\n';
    }
    return 1;
}
