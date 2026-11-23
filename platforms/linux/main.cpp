#include <memory>
#include <gtk/gtk.h>

#include "environ/cocos2d/AppDelegate.h"
#include "log/TVPInitLog.h"

#include "environ/launch/DesktopLaunch.h"
#if KRKR2_ENABLE_SDL3_HOST
#include "environ/sdl3/SdlHost.h"
#endif

int main(int argc, char **argv) {
    TVPLoggingOptions log_options;
    log_options.log_level = TVPExtractLogLevelFromArgv(argc, argv);
    TVPInitLogging(log_options);

    const int launch = krkr::launch::PrepareDesktopLaunch(argc, argv);
    if(launch >= 0)
        return launch;

    static auto pAppDelegate = std::make_unique<TVPAppDelegate>();
#if KRKR2_ENABLE_SDL3_HOST
    if(krkr::launch::UseSDL3Host())
        return krkr::sdl::Run(krkr::launch::DesktopTarget());
#endif
    gtk_init(nullptr, nullptr);
    const int result = pAppDelegate->run();
    return krkr::launch::DesktopExitCode() ? krkr::launch::DesktopExitCode()
                                           : result;
}
