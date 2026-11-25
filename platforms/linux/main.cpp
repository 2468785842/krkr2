#include <memory>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <gtk/gtk.h>

#include "environ/cocos2d/AppDelegate.h"
#include "environ/cocos2d/CocosPreferenceStorage.h"
#include "log/TVPInitLog.h"

#include "environ/launch/DesktopLaunch.h"
#if KRKR2_ENABLE_SDL3_HOST
#include "environ/ConfigManager/LocalHostServices.h"
#include "environ/sdl3/SdlHost.h"
#endif

int main(int argc, char **argv) {
    TVPLoggingOptions log_options;
    log_options.log_level = TVPExtractLogLevelFromArgv(argc, argv);
    TVPInitLogging(log_options);

#if KRKR2_ENABLE_SDL3_HOST
    std::unique_ptr<krkr::host::LocalServices> nativeServices;
#endif
    const int launch = krkr::launch::PrepareDesktopLaunch(
        argc, argv, [&](krkr::launch::DesktopHost host) {
#if KRKR2_ENABLE_SDL3_HOST
            if(host == krkr::launch::DesktopHost::SDL3) {
                nativeServices = krkr::sdl::CreateHostServices();
                return;
            }
#endif
            krkr::config::SetPreferenceStorage(
                TVPCreateCocosPreferenceStorage());
        });
    if(launch >= 0)
        return launch;

#if KRKR2_ENABLE_SDL3_HOST
    if(krkr::launch::UseSDL3Host())
        return krkr::sdl::Run(krkr::launch::DesktopTarget());
#endif
    static auto pAppDelegate = std::make_unique<TVPAppDelegate>();
    gtk_init(nullptr, nullptr);
    const int result = pAppDelegate->run();
    return krkr::launch::DesktopExitCode() ? krkr::launch::DesktopExitCode()
                                           : result;
}
