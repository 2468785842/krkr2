#include "main.h"
#include <boost/locale/encoding_utf.hpp>

#include <memory>
#include <cstdio>
#include <string>
#include <vector>

#include "tjsString.h"
#if !KRKR2_SDL3_ONLY
#include "environ/cocos2d/AppDelegate.h"
#include "environ/cocos2d/CocosPreferenceStorage.h"
#endif
#if KRKR2_ENABLE_SDL3_HOST
#include "environ/sdl3/SdlHost.h"
#endif
#include "log/TVPInitLog.h"

#include "environ/launch/DesktopLaunch.h"

int main() {
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
#if !KRKR2_SDL3_ONLY
    krkr::config::SetPreferenceStorage(TVPCreateCocosPreferenceStorage());
#endif
#if KRKR2_SDL3_ONLY
    std::unique_ptr<krkr::host::LocalServices> nativeServices;
    const int launch = krkr::launch::PrepareDesktopLaunch(
        argc, argv_ptrs.data(), [&](krkr::launch::DesktopHost) {
            nativeServices = krkr::sdl::CreateHostServices();
        });
#else
    const int launch =
        krkr::launch::PrepareDesktopLaunch(argc, argv_ptrs.data());
#endif
    if(launch >= 0)
        return launch;

#if KRKR2_ENABLE_SDL3_HOST
    if(krkr::launch::UseSDL3Host())
        return krkr::sdl::Run(krkr::launch::DesktopTarget());
#endif
#if !KRKR2_SDL3_ONLY
    static auto pAppDelegate = std::make_unique<TVPAppDelegate>();
    const int result = pAppDelegate->run();
    return krkr::launch::DesktopExitCode() ? krkr::launch::DesktopExitCode()
                                           : result;
#else
    return 2;
#endif
}
