#include <cstdlib>
#include <filesystem>
#include <memory>
#include <optional>

#include "log/TVPInitLog.h"
#include "environ/ConfigManager/LocalHostServices.h"
#include "environ/launch/DesktopLaunch.h"
#include "environ/sdl3/SdlHost.h"

int main(int argc, char **argv) {
    TVPLoggingOptions logging;
    logging.log_level = TVPExtractLogLevelFromArgv(argc, argv);
    TVPInitLogging(logging);
    std::unique_ptr<krkr::host::LocalServices> services;
    const auto prepare = [&](krkr::launch::DesktopHost) {
        services = krkr::sdl::CreateHostServices();
    };
    const int status = krkr::launch::PrepareDesktopLaunch(argc, argv, prepare);
    return status >= 0 ? status : krkr::sdl::Run(krkr::launch::DesktopTarget());
}
