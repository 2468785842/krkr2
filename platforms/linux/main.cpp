#include <memory>
#include <gtk/gtk.h>

#include "environ/cocos2d/AppDelegate.h"
#include "log/TVPInitLog.h"

#include "environ/ui/MainFileSelectorForm.h"

int main(int argc, char **argv) {
    gtk_init(&argc, &argv);

    TVPLoggingOptions log_options;
    log_options.log_level = TVPExtractLogLevelFromArgv(argc, argv);
    TVPInitLogging(log_options);

    if(argc > 1) {
        TVPMainFileSelectorForm::filePath = argv[1];
    }

    static auto pAppDelegate = std::make_unique<TVPAppDelegate>();
    return pAppDelegate->run();
}
