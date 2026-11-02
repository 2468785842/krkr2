#include "main.h"
#include <cocos2d.h>

#include <shellapi.h>
#include <boost/locale/encoding_utf.hpp>

#include <memory>
#include <cstdio>
#include <string>
#include <vector>

#include "tjsString.h"
#include "environ/cocos2d/AppDelegate.h"
#include "log/TVPInitLog.h"

#include "environ/launch/DesktopLaunch.h"
USING_NS_CC;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    int argc = 0;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if(!wargv) {
        std::fputs("krkr2: Cannot read command line\n", stderr);
        return 2;
    }
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

    LocalFree(wargv);
    const int launch =
        krkr::launch::PrepareDesktopLaunch(argc, argv_ptrs.data());
    if(launch >= 0)
        return launch;

    static auto pAppDelegate = std::make_unique<TVPAppDelegate>();
    const int result = pAppDelegate->run();
    return krkr::launch::DesktopExitCode() ? krkr::launch::DesktopExitCode()
                                           : result;
}
