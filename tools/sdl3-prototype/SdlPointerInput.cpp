#include "SdlPointerInput.h"
#include "InputModifiers.h"

#include <cmath>
#include <stdexcept>

namespace krkr::sdlprototype {
    namespace {
        std::uint32_t buttonKey(Uint8 button) {
            switch(button) {
                case SDL_BUTTON_LEFT:
                    return input::key::leftMouse;
                case SDL_BUTTON_RIGHT:
                    return input::key::rightMouse;
                case SDL_BUTTON_MIDDLE:
                    return input::key::middleMouse;
                default:
                    return 0;
            }
        }
    } // namespace

    PointerInput::PointerInput(SDL_Renderer *renderer, SDL_WindowID window,
                               input::KeyState &state) :
        renderer(renderer), window(window), state(state) {}

    bool PointerInput::accepts(SDL_EventType type) {
        return type == SDL_EVENT_MOUSE_MOTION ||
            type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
            type == SDL_EVENT_MOUSE_BUTTON_UP || type == SDL_EVENT_MOUSE_WHEEL;
    }

    void PointerInput::locate(float x, float y) {
        input::PointerPosition logical;
        if(!SDL_RenderCoordinatesFromWindow(renderer, x, y, &logical.x,
                                            &logical.y))
            throw std::runtime_error(SDL_GetError());
        position = input::pointerPixel(logical);
    }

    PointerEvent PointerInput::sample(const char *kind, std::uint32_t button,
                                      bool synthetic, float wheelX,
                                      float wheelY) {
        const auto shift = input::legacyShiftFlags(state);
        const bool current = button && state.query(button, true);
        const bool pending = button && state.query(button, false);
        return { kind,    position,  button, shift, current,
                 pending, synthetic, wheelX, wheelY };
    }

    std::optional<PointerEvent> PointerInput::handle(const SDL_Event &event,
                                                     bool focused) {
        if(!accepts(static_cast<SDL_EventType>(event.type)))
            return std::nullopt;
        SDL_WindowID source;
        if(event.type == SDL_EVENT_MOUSE_MOTION)
            source = event.motion.windowID;
        else if(event.type == SDL_EVENT_MOUSE_WHEEL)
            source = event.wheel.windowID;
        else
            source = event.button.windowID;
        if(source != window) {
            ++counts.foreign;
            return std::nullopt;
        }
        if(!focused) {
            ++counts.unfocused;
            return std::nullopt;
        }
        if(event.type == SDL_EVENT_MOUSE_MOTION) {
            locate(event.motion.x, event.motion.y);
            ++counts.moves;
            return sample("move", 0);
        }
        if(event.type == SDL_EVENT_MOUSE_WHEEL) {
            const float direction =
                event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.f : 1.f;
            const float x = direction * event.wheel.x;
            const float y = direction * event.wheel.y;
            if(!std::isfinite(x) || !std::isfinite(y))
                throw std::out_of_range("Non-finite pointer wheel delta");
            locate(event.wheel.mouse_x, event.wheel.mouse_y);
            ++counts.wheels;
            return sample("wheel", 0, false, x, y);
        }
        const auto key = buttonKey(event.button.button);
        if(!key) {
            ++counts.unsupported;
            return std::nullopt;
        }
        const bool down = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
        if(!down && !state.isDown(key)) {
            ++counts.redundantUps;
            return std::nullopt;
        }
        locate(event.button.x, event.button.y);
        if(down) {
            state.press(key);
            ++counts.downs;
        } else {
            state.release(key);
            ++counts.ups;
        }
        return sample(down ? "down" : "up", key);
    }

    std::optional<PointerEvent> PointerInput::focusRelease(std::uint32_t key) {
        if(key != input::key::leftMouse && key != input::key::rightMouse &&
           key != input::key::middleMouse)
            return std::nullopt;
        ++counts.ups;
        ++counts.syntheticUps;
        return sample("up", key, true);
    }
} // namespace krkr::sdlprototype
