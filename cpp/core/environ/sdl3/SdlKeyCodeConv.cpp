#include "SdlKeyCodeConv.h"

#include "vkdefine.h"
#include "tvpinputdefs.h"

#include <array>
#include <cstddef>

//---------------------------------------------------------------------------
// Scancode-indexed mapping table.
//
// SDL scancodes are USB HID usage IDs and therefore layout-independent; SDL
// SDL_SCANCODE_A is always the physical A position, SDL_SCANCODE_1 the top-row
// 1 key, etc. This lets the shell deliver "the key we mean" regardless of the
// current keyboard layout, which is what the engine's VK model expects.
//
// Entries are laid out with C++17-compatible constexpr construction (no
// designated initializers, which are C++20) so the table compiles with every
// toolchain used by this repository.
//---------------------------------------------------------------------------
namespace {
    constexpr tjs_uint VKNone = 0;

    constexpr std::array<tjs_uint, SDL_SCANCODE_COUNT> ScancodeVKTable = [] {
        std::array<tjs_uint, SDL_SCANCODE_COUNT> table{};
        // --- editing cluster (CCKeyCodeConv.cpp:77-112) ---
        table[SDL_SCANCODE_RETURN] = VK_RETURN;
        table[SDL_SCANCODE_ESCAPE] = VK_ESCAPE;
        table[SDL_SCANCODE_BACKSPACE] = VK_BACK;
        table[SDL_SCANCODE_TAB] = VK_TAB;
        table[SDL_SCANCODE_SPACE] = VK_SPACE;
        table[SDL_SCANCODE_CAPSLOCK] = VK_CAPITAL;
        table[SDL_SCANCODE_PRINTSCREEN] = VK_PRINT; // KEY_PRINT -> VK_PRINT
        table[SDL_SCANCODE_SCROLLLOCK] = VK_SCROLL; // KEY_SCROLL_LOCK
        table[SDL_SCANCODE_PAUSE] = VK_PAUSE;       // KEY_PAUSE / KEY_BREAK is
        // VK_CANCEL in CCKeyCodeConv.cpp:86-88; SDL cannot distinguish Break
        // from Pause, keep VK_PAUSE (the common press).

        // --- cursor / editing block (CCKeyCodeConv.cpp:109-130) ---
        table[SDL_SCANCODE_INSERT] = VK_INSERT;
        table[SDL_SCANCODE_HOME] = VK_HOME;
        table[SDL_SCANCODE_PAGEUP] = VK_PRIOR;
        table[SDL_SCANCODE_DELETE] = VK_DELETE;
        table[SDL_SCANCODE_END] = VK_END;
        table[SDL_SCANCODE_PAGEDOWN] = VK_NEXT;
        table[SDL_SCANCODE_RIGHT] = VK_RIGHT;
        table[SDL_SCANCODE_LEFT] = VK_LEFT;
        table[SDL_SCANCODE_DOWN] = VK_DOWN;
        table[SDL_SCANCODE_UP] = VK_UP;

        // --- function row (CCKeyCodeConv.cpp:65-76) ---
        table[SDL_SCANCODE_F1] = VK_F1;
        table[SDL_SCANCODE_F2] = VK_F2;
        table[SDL_SCANCODE_F3] = VK_F3;
        table[SDL_SCANCODE_F4] = VK_F4;
        table[SDL_SCANCODE_F5] = VK_F5;
        table[SDL_SCANCODE_F6] = VK_F6;
        table[SDL_SCANCODE_F7] = VK_F7;
        table[SDL_SCANCODE_F8] = VK_F8;
        table[SDL_SCANCODE_F9] = VK_F9;
        table[SDL_SCANCODE_F10] = VK_F10;
        table[SDL_SCANCODE_F11] = VK_F11;
        table[SDL_SCANCODE_F12] = VK_F12;

        // --- OEM region, US keyboard layout (CCKeyCodeConv.cpp:155-176).
        // Scancode-positioned so non-US layouts still produce the Windows VK
        // of the physical US key at that position, matching how Windows
        // defines VK_OEM_*.
        table[SDL_SCANCODE_GRAVE] = VK_OEM_3; // `~ -- NOTE: CCKeyCodeConv
        // returns 0 for KEY_GRAVE (it falls through to default), the
        // scancode-indexed table below returns the Windows VK of this key.
        // Everything else OEM matches CCKeyCodeConv exactly.
        table[SDL_SCANCODE_MINUS] = VK_OEM_MINUS;
        table[SDL_SCANCODE_EQUALS] = VK_OEM_PLUS;
        table[SDL_SCANCODE_LEFTBRACKET] = VK_OEM_4;
        table[SDL_SCANCODE_RIGHTBRACKET] = VK_OEM_6;
        table[SDL_SCANCODE_BACKSLASH] = VK_OEM_5;
        table[SDL_SCANCODE_SEMICOLON] = VK_OEM_1;
        table[SDL_SCANCODE_APOSTROPHE] = VK_OEM_7;
        table[SDL_SCANCODE_COMMA] = VK_OEM_COMMA;
        table[SDL_SCANCODE_PERIOD] = VK_OEM_PERIOD;
        table[SDL_SCANCODE_SLASH] = VK_OEM_2;
        // SDL_SCANCODE_NONUSBACKSLASH: Windows VK_OEM_102, absent from
        // vkdefine.h and unmapped by CCKeyCodeConv -> VKNone.

        // --- numeric keypad (CCKeyCodeConv.cpp:131-153).
        // The cocos host maps keypad keys to their navigation duplicates
        // (NumLock-off behavior); NUMPAD5 is the lone numpad-digit VK.
        table[SDL_SCANCODE_NUMLOCKCLEAR] = VK_NUMLOCK;
        table[SDL_SCANCODE_KP_DIVIDE] = VK_DIVIDE;
        table[SDL_SCANCODE_KP_MULTIPLY] = VK_MULTIPLY;
        table[SDL_SCANCODE_KP_MINUS] = VK_SUBTRACT;
        table[SDL_SCANCODE_KP_PLUS] = VK_ADD;
        table[SDL_SCANCODE_KP_ENTER] = VK_RETURN;
        table[SDL_SCANCODE_KP_1] = VK_END;
        table[SDL_SCANCODE_KP_2] = VK_DOWN;
        table[SDL_SCANCODE_KP_3] = VK_NEXT;
        table[SDL_SCANCODE_KP_4] = VK_LEFT;
        table[SDL_SCANCODE_KP_5] = VK_NUMPAD5;
        table[SDL_SCANCODE_KP_6] = VK_RIGHT;
        table[SDL_SCANCODE_KP_7] = VK_HOME;
        table[SDL_SCANCODE_KP_8] = VK_UP;
        table[SDL_SCANCODE_KP_9] = VK_PRIOR;
        table[SDL_SCANCODE_KP_0] = VK_INSERT;
        table[SDL_SCANCODE_KP_PERIOD] = VK_DELETE;
        // SDL_SCANCODE_KP_EQUALS / KP_COMMA: unmapped by CCKeyCodeConv.

        // --- meta keys (CCKeyCodeConv.cpp:93-108) ---
        // Left/right modifiers merge onto VK_SHIFT/VK_CONTROL/VK_MENU exactly
        // like the cocos host ("LR the same").
        table[SDL_SCANCODE_LSHIFT] = VK_SHIFT;
        table[SDL_SCANCODE_RSHIFT] = VK_SHIFT;
        table[SDL_SCANCODE_LCTRL] = VK_CONTROL;
        table[SDL_SCANCODE_RCTRL] = VK_CONTROL;
        table[SDL_SCANCODE_LALT] = VK_MENU;
        table[SDL_SCANCODE_RALT] = VK_MENU;
        table[SDL_SCANCODE_LGUI] = VK_LWIN; // KEY_HYPER -> VK_LWIN
        table[SDL_SCANCODE_RGUI] = VK_RWIN; // no cocos equivalent; natural pair
        table[SDL_SCANCODE_APPLICATION] = VK_APPS; // KEY_MENU -> VK_APPS

        // --- deliberately unmapped, aligned with CCKeyCodeConv.cpp:179-237 ---
        // SDL_SCANCODE_INTERNATIONAL1..6 (incl. the JIS Yen/Ro positions) and
        // SDL_SCANCODE_LANG1..9 (Hiragana/Katakana/Henkan/Muhenkan/...):
        // CCKeyCodeConv returns 0 for every JAPAN key including KEY_YEN, so
        // the SDL shell reports no VK either; VK_CONVERT/VK_NONCONVERT exist
        // in vkdefine.h but are unused by the current engine. F13-F24 are
        // absent from the cocos converter as well and stay 0.

        (void)VKNone;
        return table;
    }();

} // namespace

tjs_uint TVPConvertScancodeToVKCode(SDL_Scancode scancode) {
    const auto index = static_cast<std::size_t>(scancode);
    if(index < ScancodeVKTable.size() && ScancodeVKTable[index])
        return ScancodeVKTable[index];

    // Alphanumeric region: tools/sdl3-prototype/main.cpp:64-93 strategy.
    // SDL_GetKeyFromScancode with KMOD_NONE strips modifiers and dead-key
    // composition, and key_event=true yields the base keycode for that
    // physical key under the current layout, so Ctrl+Shift+A still reaches
    // the engine as VK_A.
    const SDL_Keycode key =
        SDL_GetKeyFromScancode(scancode, SDL_KMOD_NONE, true);
    // KMOD_NONE always yields the lowercase base keycode, so normalize to the
    // uppercase range the engine VK table is keyed by.
    if(key >= SDLK_a && key <= SDLK_z)
        return static_cast<tjs_uint>(VK_A + (key - SDLK_a));
    if(key >= SDLK_0 && key <= SDLK_9)
        return static_cast<tjs_uint>(VK_0 + (key - SDLK_0));
    return 0;
}

tjs_uint TVPConvertSDLMouseButtonToVKCode(Uint8 button) {
    switch(button) {
        case SDL_BUTTON_LEFT:
            return VK_LBUTTON;
        case SDL_BUTTON_MIDDLE:
            return VK_MBUTTON;
        case SDL_BUTTON_RIGHT:
            return VK_RBUTTON;
        default:
            return 0; // X1/X2 and beyond: no engine VK (CCKeyCodeConv returns 0)
    }
}

tjs_uint TVPConvertSDLGamepadToVK(SDL_GamepadButton button) {
    switch(button) {
        // Face buttons; SDL names follow the Xbox layout (South = A).
        // CCKeyCodeConv.cpp:244-254 maps BUTTON_A..Z to VK_PAD1..6.
        case SDL_GAMEPAD_BUTTON_SOUTH:
            return VK_PAD1; // A
        case SDL_GAMEPAD_BUTTON_EAST:
            return VK_PAD2; // B
        // No CC equivalent for the extra face buttons C/Z -> no VK_PAD3/6.
        case SDL_GAMEPAD_BUTTON_WEST:
            return VK_PAD4; // X
        case SDL_GAMEPAD_BUTTON_NORTH:
            return VK_PAD5; // Y
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            return VK_PAD7;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            return VK_PAD8;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK:
            return VK_PAD9;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
            return VK_PAD10;
        // START/BACK alias the stick VKs exactly like CCKeyCodeConv.cpp:264-267
        // (BUTTON_START -> VK_PAD9, BUTTON_SELECT -> VK_PAD10).
        case SDL_GAMEPAD_BUTTON_START:
            return VK_PAD9;
        case SDL_GAMEPAD_BUTTON_BACK:
            return VK_PAD10;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            return VK_PADUP;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            return VK_PADDOWN;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            return VK_PADLEFT;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            return VK_PADRIGHT;
        // GUIDE/MISC1/touchpad/paddles have no cocos equivalent; the engine's
        // VK_PAD space (0x1B0-0x1DF) has no free entry assigned for them.
        default:
            return 0;
    }
}

tjs_uint TVPConvertSDLGamepadAxisToVK(SDL_GamepadAxis axis) {
    // TODO(B3): see header. Only the triggers exist in the cocos host
    // (CCKeyCodeConv.cpp:268-271); sticks remain analog and need a
    // threshold policy before they can become VK_PADLEFT/... events.
    switch(axis) {
        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
            return VK_PAD5;
        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
            return VK_PAD6;
        default:
            return 0;
    }
}
