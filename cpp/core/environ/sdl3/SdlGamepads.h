#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

namespace krkr::sdl {
    class Gamepads {
    public:
        struct Transition {
            unsigned key;
            bool down;
            bool cancellation = false;
        };

        Gamepads();
        ~Gamepads();

        bool Handle(const SDL_Event &event, bool acceptsInput,
                    std::vector<Transition> &transitions);
        void CancelInput();

    private:
        enum Source {
            Left = SDL_GAMEPAD_BUTTON_COUNT,
            Up,
            Right,
            Down,
            LeftTrigger,
            RightTrigger,
            SourceCount
        };

        struct Control {
            bool physical = false;
            bool delivered = false;
        };

        struct Device {
            std::unique_ptr<SDL_Gamepad, decltype(&SDL_CloseGamepad)> gamepad{
                nullptr, SDL_CloseGamepad
            };
            std::array<Control, SourceCount> controls{};
        };

        bool enabled = false;
        std::unordered_map<SDL_JoystickID, Device> devices;
        std::array<unsigned, 0x200> pressed{};

        void SetControl(Device &device, int source, bool down,
                        bool acceptsInput,
                        std::vector<Transition> &transitions);
        void Release(Device &device, std::vector<Transition> &transitions);
        void Remove(SDL_JoystickID id, std::vector<Transition> &transitions);
        void UpdateAxis(Device &device, const SDL_GamepadAxisEvent &axis,
                        bool acceptsInput,
                        std::vector<Transition> &transitions);
        static unsigned VirtualKey(int source);
    };
} // namespace krkr::sdl
