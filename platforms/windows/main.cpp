#include "main.h"
#include <cocos2d.h>

#include <spdlog/spdlog.h>
#include <shellapi.h>
#include <boost/locale.hpp>

#include <string>
#include <vector>

#include "tjsString.h"
#include "environ/cocos2d/AppDelegate.h"
#include "log/TVPInitLog.h"

#include "environ/ui/MainFileSelectorForm.h"
USING_NS_CC;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    int argc = 0;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    std::vector<std::string> utf8_args;
    utf8_args.reserve(static_cast<size_t>(argc));
    std::vector<char *> argv_ptrs;
    argv_ptrs.reserve(static_cast<size_t>(argc));
    for(int i = 0; i < argc; ++i) {
        utf8_args.push_back(
            boost::locale::conv::utf_to_utf<char>(std::wstring(wargv[i])));
        argv_ptrs.push_back(utf8_args.back().data());
    }

    TVPLoggingOptions log_options;
    log_options.log_level = TVPExtractLogLevelFromArgv(argc, argv_ptrs.data());
    TVPInitLogging(log_options);

    // 处理命令行参数，获取拖拽的文件路径
    if(argc > 1) {
        std::string xp3PathUtf8 = utf8_args[1];
        spdlog::info("XP3 文件路径: {}", xp3PathUtf8);
        TVPMainFileSelectorForm::filePath = xp3PathUtf8;
    }

    LocalFree(wargv);

    static auto pAppDelegate = std::make_unique<TVPAppDelegate>();
    return pAppDelegate->run();
}
