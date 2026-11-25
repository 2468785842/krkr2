#pragma once

#include <string>
#include "VideoPresentation.h"
#include "ConfigManager/LocalHostServices.h"

class iWindowLayer;
class tTJSNI_Window;

namespace krkr::sdl {
    std::unique_ptr<krkr::host::LocalServices> CreateHostServices();
    void Capture(const std::string &path, std::function<void(bool)> completion);
    int DrawOnce(int interval);
    int Run(const std::string &game);
    void ReportPreferenceWriteFailure();
    bool Active();
    iWindowLayer *CreateGameWindow(tTJSNI_Window *window);
    bool GetScreenSize(int &width, int &height);
    std::unique_ptr<video::Presentation>
    CreateVideoPresentation(iWindowLayer &window, video::FrameCallback onFrame);
} // namespace krkr::sdl
