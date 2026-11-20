#pragma once

//---------------------------------------------------------------------------
// SDL3 scancode / mouse button / gamepad -> Windows-style VK conversion.
//
// Replaces environ/cocos2d/CCKeyCodeConv for the SDL3 shell. The table is
// indexed by SDL_Scancode, so mapping is layout-independent and key-position
// fixed, unlike the keycode-based cocos converter. Behavior parity targets
// CCKeyCodeConv.cpp; the alphanumeric region follows the verified prototype
// strategy tools/sdl3-prototype/main.cpp:64-93 (SDL_GetKeyFromScancode with
// KMOD_NONE so modifiers/dead-keys cannot rewrite the VK).
//
// No cocos headers are referenced here.
//---------------------------------------------------------------------------

#include <SDL3/SDL.h>

#include "tjsTypes.h"

// SDL physical scancode -> Windows VK. Returns 0 for unmapped keys, matching
// CCKeyCodeConv.cpp:236-237 (e.g. all JAPAN/international keys, F13-F24 and
// multimedia keys stay unmapped).
tjs_uint TVPConvertScancodeToVKCode(SDL_Scancode scancode);

// SDL mouse button (SDL_BUTTON_*) -> VK_LBUTTON/VK_RBUTTON/VK_MBUTTON.
// Replaces TVPConvertMouseBtnToVKCode (CCKeyCodeConv.cpp:5-22).
tjs_uint TVPConvertSDLMouseButtonToVKCode(Uint8 button);

// SDL gamepad button -> VK_PAD1..VK_PAD10 / VK_PADUP..VK_PADDOWN
// (tvpinputdefs.h:60-78), mirroring TVPConvertPadKeyCodeToVKCode
// (CCKeyCodeConv.cpp:242-286).
tjs_uint TVPConvertSDLGamepadToVK(SDL_GamepadButton button);

// SDL gamepad axis -> VK_PAD*, or 0. The cocos host only mapped the analog
// triggers (CCKeyCodeConv.cpp:268-271, AXIS_LEFT_TRIGGER -> VK_PAD5,
// AXIS_RIGHT_TRIGGER -> VK_PAD6). TODO(B3): the engine VK layer is binary
// while SDL delivers analog values; the caller (SdlAppDelegate) must decide
// a press/release threshold policy before these VKs are posted.
tjs_uint TVPConvertSDLGamepadAxisToVK(SDL_GamepadAxis axis);
