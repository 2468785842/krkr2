#include "SdlGamepads.h"

#include "TVPLog.h"
#include "tvpinputdefs.h"

#include <utility>

namespace krkr::sdl {
    Gamepads::Gamepads() {
        // Keep tracking hardware in the background. Focus ownership and
        // cancellation are resolved in the host, even with several windows.
        SDL_SetHintWithPriority(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1",
                                SDL_HINT_OVERRIDE);
        enabled = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
        if(!enabled)
            G_CoreLog.warn("SDL3 gamepad initialization failed: {}",
                           SDL_GetError());
    }

    Gamepads::~Gamepads() {
        devices.clear();
        if(enabled)
            SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    }

    unsigned Gamepads::VirtualKey(int source) {
        switch(source) {
            case SDL_GAMEPAD_BUTTON_SOUTH:
                return VK_PAD1;
            case SDL_GAMEPAD_BUTTON_EAST:
                return VK_PAD2;
            case SDL_GAMEPAD_BUTTON_WEST:
                return VK_PAD4;
            case SDL_GAMEPAD_BUTTON_NORTH:
            case LeftTrigger:
                return VK_PAD5;
            case RightTrigger:
                return VK_PAD6;
            case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
                return VK_PAD7;
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
                return VK_PAD8;
            case SDL_GAMEPAD_BUTTON_START:
            case SDL_GAMEPAD_BUTTON_LEFT_STICK:
                return VK_PAD9;
            case SDL_GAMEPAD_BUTTON_BACK:
            case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
                return VK_PAD10;
            case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            case Left:
                return VK_PADLEFT;
            case SDL_GAMEPAD_BUTTON_DPAD_UP:
            case Up:
                return VK_PADUP;
            case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            case Right:
                return VK_PADRIGHT;
            case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            case Down:
                return VK_PADDOWN;
            default:
                return 0;
        }
    }

    void Gamepads::SetControl(Device &device, int source, bool down,
                              bool acceptsInput,
                              std::vector<Transition> &transitions) {
        const auto key = VirtualKey(source);
        if(!key)
            return;
        auto &control = device.controls[source];
        if(control.physical == down)
            return;
        control.physical = down;
        if(down) {
            if(acceptsInput) {
                control.delivered = true;
                if(pressed[key]++ == 0)
                    transitions.push_back({ key, true });
            }
        } else if(control.delivered) {
            control.delivered = false;
            if(--pressed[key] == 0)
                transitions.push_back({ key, false });
        }
    }

    void Gamepads::UpdateAxis(Device &device, const SDL_GamepadAxisEvent &axis,
                              bool acceptsInput,
                              std::vector<Transition> &transitions) {
        const auto update = [&](Source source, int value) {
            constexpr int pressThreshold = 16384;
            constexpr int releaseThreshold = 12288;
            const auto threshold = device.controls[source].physical
                ? releaseThreshold
                : pressThreshold;
            SetControl(device, source, value >= threshold, acceptsInput,
                       transitions);
        };

        switch(axis.axis) {
            case SDL_GAMEPAD_AXIS_LEFTX:
            case SDL_GAMEPAD_AXIS_LEFTY: {
                const auto negative =
                    axis.axis == SDL_GAMEPAD_AXIS_LEFTX ? Left : Up;
                const auto positive =
                    axis.axis == SDL_GAMEPAD_AXIS_LEFTX ? Right : Down;
                // Release the previous direction before crossing to its
                // opposite.
                if(axis.value >= 0) {
                    update(negative, -axis.value);
                    update(positive, axis.value);
                } else {
                    update(positive, axis.value);
                    update(negative, -axis.value);
                }
                break;
            }
            case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                update(LeftTrigger, axis.value);
                break;
            case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                update(RightTrigger, axis.value);
                break;
        }
    }

    void Gamepads::Release(Device &device,
                           std::vector<Transition> &transitions) {
        for(int source = 0; source < SourceCount; ++source) {
            auto &control = device.controls[source];
            if(control.delivered) {
                control.delivered = false;
                const auto key = VirtualKey(source);
                if(--pressed[key] == 0)
                    transitions.push_back({ key, false, true });
            }
        }
    }

    void Gamepads::Remove(SDL_JoystickID id,
                          std::vector<Transition> &transitions) {
        const auto device = devices.find(id);
        if(device != devices.end()) {
            Release(device->second, transitions);
            devices.erase(device);
        }
    }

    void Gamepads::CancelInput() {
        pressed.fill(0);
        for(auto &[id, device] : devices)
            for(auto &control : device.controls)
                control.delivered = false;
    }

    bool Gamepads::Handle(const SDL_Event &event, bool acceptsInput,
                          std::vector<Transition> &transitions) {
        SDL_JoystickID id;
        switch(event.type) {
            case SDL_EVENT_GAMEPAD_ADDED: {
                id = event.gdevice.which;
                if(!devices.count(id)) {
                    Device device;
                    device.gamepad.reset(SDL_OpenGamepad(id));
                    if(device.gamepad)
                        devices.emplace(id, std::move(device));
                }
                return true;
            }
            case SDL_EVENT_GAMEPAD_REMOVED:
                Remove(event.gdevice.which, transitions);
                return true;
            case SDL_EVENT_GAMEPAD_REMAPPED:
                id = event.gdevice.which;
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                id = event.gbutton.which;
                break;
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                id = event.gaxis.which;
                break;
            default:
                return false;
        }

        const auto found = devices.find(id);
        if(found == devices.end())
            return true;
        auto &device = found->second;
        // SDL can queue recenter events before its device-removed event.
        if(!SDL_GamepadConnected(device.gamepad.get())) {
            Remove(id, transitions);
        } else if(event.type == SDL_EVENT_GAMEPAD_REMAPPED) {
            Release(device, transitions);
            for(int button = 0; button < SDL_GAMEPAD_BUTTON_COUNT; ++button)
                device.controls[button].physical = SDL_GetGamepadButton(
                    device.gamepad.get(),
                    static_cast<SDL_GamepadButton>(button));
            for(const auto axis :
                { SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY,
                  SDL_GAMEPAD_AXIS_LEFT_TRIGGER,
                  SDL_GAMEPAD_AXIS_RIGHT_TRIGGER }) {
                SDL_GamepadAxisEvent sample{};
                sample.axis = axis;
                sample.value = SDL_GetGamepadAxis(device.gamepad.get(), axis);
                UpdateAxis(device, sample, false, transitions);
            }
        } else if(event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION) {
            UpdateAxis(device, event.gaxis, acceptsInput, transitions);
        } else if(event.gbutton.button < SDL_GAMEPAD_BUTTON_COUNT) {
            SetControl(device, event.gbutton.button, event.gbutton.down,
                       acceptsInput, transitions);
        }
        return true;
    }
} // namespace krkr::sdl
