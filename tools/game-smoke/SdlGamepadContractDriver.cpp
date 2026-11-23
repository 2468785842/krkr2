// Compiled only into explicit UI contract builds.
#include <SDL3/SDL.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

void TVPUpdateSDLGamepadContract(const std::filesystem::path &request) {
    static std::array<SDL_Joystick *, 2> pads{};
    std::ifstream input(request);
    if(!input)
        return;

    unsigned sequence, index;
    std::string action;
    if(!(input >> sequence >> action >> index) || index >= pads.size())
        throw std::runtime_error("Invalid SDL gamepad contract request");
    auto &pad = pads[index];

    if(action == "attach" && !pad) {
        SDL_VirtualJoystickDesc description;
        SDL_INIT_INTERFACE(&description);
        description.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        description.name = "KrKr2 virtual test gamepad";
        description.naxes = SDL_GAMEPAD_AXIS_COUNT;
        description.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
        const auto id = SDL_AttachVirtualJoystick(&description);
        pad = id ? SDL_OpenJoystick(id) : nullptr;
        if(!pad)
            throw std::runtime_error(SDL_GetError());
    } else if(action == "detach" && pad) {
        if(!SDL_DetachVirtualJoystick(SDL_GetJoystickID(pad)))
            throw std::runtime_error(SDL_GetError());
        SDL_CloseJoystick(pad);
        pad = nullptr;
    } else if((action == "button" || action == "axis") && pad) {
        int control, value;
        if(!(input >> control >> value))
            throw std::runtime_error("Missing SDL gamepad test value");
        const bool applied = action == "button"
            ? SDL_SetJoystickVirtualButton(pad, control, value != 0)
            : SDL_SetJoystickVirtualAxis(pad, control, value);
        if(!applied)
            throw std::runtime_error(SDL_GetError());
    } else {
        throw std::runtime_error("Unexpected SDL gamepad contract action");
    }

    SDL_UpdateJoysticks();
    input.close();
    std::filesystem::remove(request);
    std::ofstream ack(request.parent_path() / "sdl-pad-ack.txt");
    ack << sequence << '\n';
}
