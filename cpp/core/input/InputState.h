#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace krkr::input {
    // Virtual-key state shared by keyboard, pointer and controller adapters.
    // A current-state query also consumes the pending press, matching the
    // existing System.getKeyState behavior.
    class KeyState {
    public:
        void press(std::uint32_t key);
        bool release(std::uint32_t key);
        // Release every held key before dispatching focus loss. Preserve unread
        // presses, just like release(); return keys in virtual-key order so the
        // adapter can send matching key-up events after all state is updated.
        std::vector<std::uint32_t> releaseAll();
        void latchReleasedPress(std::uint32_t key);
        bool isDown(std::uint32_t key) const;
        bool query(std::uint32_t key, bool current);

    private:
        std::array<std::uint8_t, 0x200> state{};
    };

    // Process-wide input state; access remains on the engine/input thread.
    KeyState &sharedState();
} // namespace krkr::input
