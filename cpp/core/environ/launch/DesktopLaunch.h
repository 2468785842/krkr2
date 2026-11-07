#pragma once

#include <string>

namespace krkr::launch {
    // -1: continue runtime startup; 0: help printed; 2: invalid invocation.
    int PrepareDesktopLaunch(int argc, char **argv);
    const std::string &DesktopTarget();
    bool ValidateDesktopArchive();
    int DesktopExitCode();
} // namespace krkr::launch
