#pragma once

#include <SDL3/SDL.h>

namespace krkr::sdlprototype {
    inline constexpr unsigned pointerTestFrames = 8;
    const char *pointerCaptureName(unsigned stage);
    void pushPointerTest(SDL_WindowID window, unsigned stage);
} // namespace krkr::sdlprototype
