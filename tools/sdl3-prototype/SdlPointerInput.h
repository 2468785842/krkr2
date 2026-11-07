#pragma once

#include <SDL3/SDL.h>
#include "InputState.h"
#include "PointerCoordinates.h"

#include <optional>

namespace krkr::sdlprototype {
    struct PointerEvent {
        const char *kind;
        input::PointerPixel position;
        std::uint32_t button, shift;
        bool current, pending, synthetic;
        float wheelX, wheelY;
    };

    struct PointerStats {
        unsigned moves = 0, downs = 0, ups = 0, wheels = 0;
        unsigned foreign = 0, unfocused = 0, redundantUps = 0, unsupported = 0;
        unsigned syntheticUps = 0;
    };

    // Experiment adapter; SDL owns window/DPI/viewport conversion. The shared
    // input layer owns VK state, shift flags and integer coordinate semantics.
    class PointerInput {
    public:
        PointerInput(SDL_Renderer *renderer, SDL_WindowID window,
                     input::KeyState &state);
        static bool accepts(SDL_EventType type);
        std::optional<PointerEvent> handle(const SDL_Event &event,
                                           bool focused);
        // Called after shared KeyState::releaseAll(), before focus
        // notification.
        std::optional<PointerEvent> focusRelease(std::uint32_t key);
        const PointerStats &stats() const { return counts; }

    private:
        PointerEvent sample(const char *kind, std::uint32_t button,
                            bool synthetic = false, float wheelX = 0,
                            float wheelY = 0);
        void locate(float x, float y);
        SDL_Renderer *renderer;
        SDL_WindowID window;
        input::KeyState &state;
        input::PointerPixel position{ 0, 0 };
        PointerStats counts;
    };
} // namespace krkr::sdlprototype
