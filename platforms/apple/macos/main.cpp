#include <memory>
#include <cocos2d.h>

#include "environ/cocos2d/AppDelegate.h"
#include "log/TVPInitLog.h"
#include "ui/MainFileSelectorForm.h"

USING_NS_CC;

int main(int argc, char *argv[]) {
    TVPLoggingOptions log_options;
    log_options.log_level = TVPExtractLogLevelFromArgv(argc, argv);
    TVPInitLogging(log_options);

    if(argc > 1) {
        TVPMainFileSelectorForm::filePath = argv[1];
    }

    static std::unique_ptr<TVPAppDelegate> pAppDelegate =
        std::make_unique<TVPAppDelegate>();
    return pAppDelegate->run();
}
