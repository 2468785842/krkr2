#pragma once

#include <string>

class iWindowLayer;
class tTJSNI_Window;

namespace krkr::sdl {
    int Run(const std::string &game);
    bool Active();
    iWindowLayer *CreateWindow(tTJSNI_Window *window);
    bool GetScreenSize(int &width, int &height);
} // namespace krkr::sdl
