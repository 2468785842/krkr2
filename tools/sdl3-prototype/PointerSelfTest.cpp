#include "PointerSelfTest.h"

#include <array>
#include <stdexcept>

namespace krkr::sdlprototype {
    namespace {
        void push(SDL_Event event) {
            if(!SDL_PushEvent(&event))
                throw std::runtime_error(SDL_GetError());
        }

        void motion(SDL_WindowID window, float x, float y) {
            SDL_Event event{};
            event.type = SDL_EVENT_MOUSE_MOTION;
            event.motion.windowID = window;
            event.motion.x = x;
            event.motion.y = y;
            push(event);
        }

        void button(SDL_WindowID window, bool down, Uint8 button, float x,
                    float y) {
            SDL_Event event{};
            event.type =
                down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
            event.button.windowID = window;
            event.button.button = button;
            event.button.down = down;
            event.button.x = x;
            event.button.y = y;
            push(event);
        }

        void wheel(SDL_WindowID window) {
            SDL_Event event{};
            event.type = SDL_EVENT_MOUSE_WHEEL;
            event.wheel.windowID = window;
            event.wheel.mouse_x = 280;
            event.wheel.mouse_y = 220;
            event.wheel.x = .5f;
            event.wheel.y = -1.5f;
            event.wheel.direction = SDL_MOUSEWHEEL_FLIPPED;
            push(event);
        }

        void windowEvent(SDL_WindowID window, SDL_EventType type) {
            SDL_Event event{};
            event.type = type;
            event.window.windowID = window;
            push(event);
        }
    } // namespace

    const char *pointerCaptureName(unsigned stage) {
        static constexpr std::array<const char *, pointerTestFrames> names{
            "initial.bmp",          "pointer-down.bmp",
            "pointer-drag.bmp",     "pointer-up.bmp",
            "pointer-wheel.bmp",    "pointer-unfocused.bmp",
            "pointer-restored.bmp", "pointer-veto.bmp"
        };
        return names.at(stage);
    }

    void pushPointerTest(SDL_WindowID window, unsigned stage) {
        switch(stage) {
            case 0:
                motion(window, 180, 120);
                button(window, true, SDL_BUTTON_LEFT, 180, 120);
                break;
            case 1:
                motion(window, 40, 120); // Left letterbox bar -> x = -40.
                break;
            case 2:
                motion(window, 850, 120); // Outside the window, no clipping.
                button(window, false, SDL_BUTTON_LEFT, 850, 120);
                break;
            case 3:
                wheel(window);
                break;
            case 4:
                button(window, true, SDL_BUTTON_RIGHT, 280, 220);
                windowEvent(window, SDL_EVENT_WINDOW_FOCUS_LOST);
                break;
            case 5:
                button(window, true, SDL_BUTTON_LEFT, 180, 120);
                wheel(window);
                motion(0, 0, 0); // A different window must not move the cursor.
                windowEvent(window, SDL_EVENT_WINDOW_FOCUS_GAINED);
                button(window, false, SDL_BUTTON_LEFT, 0, 0); // Redundant up.
                break;
            case 6:
            case 7:
                windowEvent(window, SDL_EVENT_WINDOW_CLOSE_REQUESTED);
                break;
            default:
                throw std::out_of_range("Invalid pointer self-test stage");
        }
    }
} // namespace krkr::sdlprototype
