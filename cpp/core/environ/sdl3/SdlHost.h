#pragma once

#include <string>
#include "VideoPresentation.h"

class iWindowLayer;
class tTJSNI_Window;

namespace krkr::sdl {
    int Run(const std::string &game);
    bool Active();
    iWindowLayer *CreateWindow(tTJSNI_Window *window);
    bool GetScreenSize(int &width, int &height);
    std::unique_ptr<video::Presentation>
    CreateVideoPresentation(iWindowLayer &window, video::FrameCallback onFrame);
} // namespace krkr::sdl
