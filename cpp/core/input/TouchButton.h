#pragma once

#include "InputModifiers.h"
#include <cstddef>
#include <optional>

namespace krkr::input {
    // One gesture's button, independent of physical mouse events and windows.
    // Contacts select left/right/middle until the first actual down. Additional
    // fingers cannot change a held button; the final up must match that down.
    class TouchButton {
    public:
        bool selectForContacts(std::size_t contacts) {
            if(down || contacts < 1 || contacts > 3)
                return false;
            constexpr std::uint32_t buttons[] = { key::leftMouse,
                                                  key::rightMouse,
                                                  key::middleMouse };
            selected = buttons[contacts - 1];
            return true;
        }
        std::uint32_t value() const { return selected; }
        bool pressed() const { return down; }
        std::optional<std::uint32_t> press() {
            if(down)
                return std::nullopt;
            down = true;
            return selected;
        }
        // Only call when the last contact ends/cancels. A deferred tap has no
        // held button to release; the adapter handles its click separately.
        std::optional<std::uint32_t> finish() {
            const auto released =
                down ? std::optional{ selected } : std::nullopt;
            reset();
            return released;
        }
        void reset() {
            selected = key::leftMouse;
            down = false;
        }

    private:
        std::uint32_t selected = key::leftMouse;
        bool down = false;
    };
} // namespace krkr::input
