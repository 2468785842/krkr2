// Compiled only into explicit UI contract builds.
#include <SDL3/SDL.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

bool TVPSubmitSDLTouchContract(const std::filesystem::path &request,
                               SDL_WindowID window) {
    std::ifstream input(request);
    if(!input)
        return false;

    std::vector<SDL_Event> events;
    std::string action;
    while(input >> action) {
        SDL_Event event{};
        auto &touch = event.tfinger;
        if(action == "down")
            touch.type = SDL_EVENT_FINGER_DOWN;
        else if(action == "move")
            touch.type = SDL_EVENT_FINGER_MOTION;
        else if(action == "up")
            touch.type = SDL_EVENT_FINGER_UP;
        else if(action == "cancel")
            touch.type = SDL_EVENT_FINGER_CANCELED;
        else
            throw std::runtime_error("Invalid SDL touch contract action");
        if(!(input >> touch.touchID >> touch.fingerID >> touch.x >> touch.y))
            throw std::runtime_error("Incomplete SDL touch contract sample");
        touch.windowID = window;
        events.push_back(event);
    }
    for(auto &event : events)
        if(!SDL_PushEvent(&event))
            throw std::runtime_error(SDL_GetError());
    return true;
}
