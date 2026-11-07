#include "InputModifiers.h"
#include "InputState.h"

namespace krkr::input {
    std::uint32_t legacyShiftFlags(const KeyState &state) {
        std::uint32_t flags = 0;
        if(state.isDown(key::shift))
            flags |= modifier::shift;
        if(state.isDown(key::alt))
            flags |= modifier::alt;
        if(state.isDown(key::control))
            flags |= modifier::control;
        if(state.isDown(key::leftMouse))
            flags |= modifier::leftMouse;
        if(state.isDown(key::rightMouse))
            flags |= modifier::rightMouse;
        return flags;
    }
} // namespace krkr::input
