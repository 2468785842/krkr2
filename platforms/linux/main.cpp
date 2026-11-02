#include <memory>
#include <gtk/gtk.h>

#include "environ/cocos2d/AppDelegate.h"
#include "log/TVPInitLog.h"

#include "environ/launch/DesktopLaunch.h"

int main(int argc, char **argv) {
    TVPLoggingOptions log_options;
    log_options.log_level = TVPExtractLogLevelFromArgv(argc, argv);
    TVPInitLogging(log_options);

    const int launch = krkr::launch::PrepareDesktopLaunch(argc, argv);
    if(launch >= 0)
        return launch;

    gtk_init(nullptr, nullptr);

    static auto pAppDelegate = std::make_unique<TVPAppDelegate>();
    const int result = pAppDelegate->run();
    return krkr::launch::DesktopExitCode() ? krkr::launch::DesktopExitCode()
                                           : result;
}
