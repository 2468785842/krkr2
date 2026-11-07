#pragma once

#include <cstdint>
#include <vector>

namespace krkr::input {
    // Legacy mouse-key bindings: Return/Space/Pad1 -> left, Escape/Pad2 ->
    // right. Returns a mouse VK, or zero for an ordinary keyboard/controller
    // key.
    std::uint32_t mouseKeyButton(std::uint32_t key);

    struct MouseKeyState {
        bool enabled = false;
        bool leftPressed = false;
        bool rightPressed = false;
    };

    struct InputCancellation {
        std::vector<std::uint32_t> pointerUps;
        std::vector<std::uint32_t> keyUps;
    };

    // Call after releaseAll(). Preserve its key order, deduplicate emulated and
    // physical buttons, and never invent an up for an untranslated mouse-key
    // press. This plan contains releases only; cancellation cannot click.
    InputCancellation
    planInputCancellation(const std::vector<std::uint32_t> &released,
                          MouseKeyState mouseKeys);
} // namespace krkr::input
