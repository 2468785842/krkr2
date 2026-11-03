#include "InputState.h"

namespace krkr::input {
    namespace {
        constexpr std::uint8_t down = 0x01;
        constexpr std::uint8_t pendingPress = 0x10;
    } // namespace

    void KeyState::press(std::uint32_t key) {
        if(key < state.size())
            state[key] = down | pendingPress;
    }

    bool KeyState::release(std::uint32_t key) {
        if(key >= state.size())
            return false;
        const bool wasDown = (state[key] & down) != 0;
        state[key] &= pendingPress;
        return wasDown;
    }

    std::vector<std::uint32_t> KeyState::releaseAll() {
        std::vector<std::uint32_t> released;
        for(std::uint32_t key = 0; key < state.size(); ++key) {
            if(isDown(key))
                released.push_back(key);
        }
        for(auto key : released)
            release(key);
        return released;
    }

    void KeyState::latchReleasedPress(std::uint32_t key) {
        if(key < state.size())
            state[key] = pendingPress;
    }

    bool KeyState::isDown(std::uint32_t key) const {
        return key < state.size() && (state[key] & down) != 0;
    }

    bool KeyState::query(std::uint32_t key, bool current) {
        if(key >= state.size())
            return false;
        const auto previous = state[key];
        state[key] &= down;
        return (previous & (current ? down : pendingPress)) != 0;
    }

    KeyState &sharedState() {
        static KeyState instance;
        return instance;
    }
} // namespace krkr::input
