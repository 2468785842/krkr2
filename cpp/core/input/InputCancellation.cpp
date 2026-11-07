#include "InputCancellation.h"
#include "InputModifiers.h"

namespace krkr::input {
    std::uint32_t mouseKeyButton(std::uint32_t value) {
        switch(value) {
            case 0x0d: // Return.
            case 0x20: // Space.
            case 0x1c0: // Pad1.
                return key::leftMouse;
            case 0x1b: // Escape.
            case 0x1c1: // Pad2.
                return key::rightMouse;
            default:
                return 0;
        }
    }

    InputCancellation
    planInputCancellation(const std::vector<std::uint32_t> &released,
                          MouseKeyState mouseKeys) {
        InputCancellation plan;
        bool left = mouseKeys.leftPressed;
        bool right = mouseKeys.rightPressed;
        bool middle = false;
        for(auto value : released) {
            if(value == key::leftMouse)
                left = true;
            else if(value == key::rightMouse)
                right = true;
            else if(value == key::middleMouse)
                middle = true;
            else if(!mouseKeys.enabled || !mouseKeyButton(value))
                plan.keyUps.push_back(value);
        }
        if(left)
            plan.pointerUps.push_back(key::leftMouse);
        if(right)
            plan.pointerUps.push_back(key::rightMouse);
        if(middle)
            plan.pointerUps.push_back(key::middleMouse);
        return plan;
    }
} // namespace krkr::input
