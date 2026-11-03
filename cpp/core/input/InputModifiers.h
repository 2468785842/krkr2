#pragma once

#include <cstdint>

namespace krkr::input {
    class KeyState;

    namespace key {
        inline constexpr std::uint32_t leftMouse = 0x01;
        inline constexpr std::uint32_t rightMouse = 0x02;
        inline constexpr std::uint32_t middleMouse = 0x04;
        inline constexpr std::uint32_t shift = 0x10;
        inline constexpr std::uint32_t control = 0x11;
        inline constexpr std::uint32_t alt = 0x12;
    } // namespace key

    namespace modifier {
        inline constexpr std::uint32_t shift = 0x01;
        inline constexpr std::uint32_t alt = 0x02;
        inline constexpr std::uint32_t control = 0x04;
        inline constexpr std::uint32_t leftMouse = 0x08;
        inline constexpr std::uint32_t rightMouse = 0x10;
    } // namespace modifier

    // A non-consuming snapshot of the existing engine shift flags. The old
    // host deliberately omitted the middle button; preserve that behavior.
    // Repeat/double-click are event flags, not part of a state snapshot.
    std::uint32_t legacyShiftFlags(const KeyState &state);
} // namespace krkr::input
