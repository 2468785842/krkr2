#pragma once

#include <functional>
#include <string>

#include "LaunchOptions.h"

namespace krkr::launch {
    // -1: continue runtime startup; 0: help printed; 2: invalid invocation.
    // Called after parsing the host and before archive validation can read
    // configuration. The caller retains ownership of the installed services.
    using PrepareHost = std::function<void(DesktopHost)>;
    int PrepareDesktopLaunch(int argc, char **argv,
                             const PrepareHost &prepareHost = {});
    const std::string &DesktopTarget();
    bool UseSDL3Host();
    bool ValidateDesktopArchive();
    int DesktopExitCode();
} // namespace krkr::launch
