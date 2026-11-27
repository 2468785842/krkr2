#include "main.h"
#include <boost/locale/encoding_utf.hpp>

#include <memory>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#if defined(_DEBUG)
#include <crtdbg.h>
#endif

#include "tjsString.h"
#include "environ/sdl3/SdlHost.h"
#include "log/TVPInitLog.h"

#include "environ/launch/DesktopLaunch.h"

namespace {
    // Debug CRT 断言默认弹模态框；自动化运行（smoke 脚本、CI、重定向日志）
    // 没有人在窗口前点“继续”，进程会永久挂住，看起来像脚本静默失败。
    // 转成 stderr 输出后断言与 abort 仍以非零退出码失败。
    void ConfigureDebugCrtReporting() {
#if defined(_DEBUG)
        _set_error_mode(_OUT_TO_STDERR);
        _set_abort_behavior(0, _CALL_REPORTFAULT);
        for(const int type : { _CRT_WARN, _CRT_ERROR, _CRT_ASSERT }) {
            _CrtSetReportMode(type, _CRTDBG_MODE_FILE);
            _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
        }
#endif
    }
} // namespace

int main() {
    ConfigureDebugCrtReporting();
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
    std::unique_ptr<krkr::host::LocalServices> nativeServices;
    const int launch = krkr::launch::PrepareDesktopLaunch(
        argc, argv_ptrs.data(), [&](krkr::launch::DesktopHost) {
            nativeServices = krkr::sdl::CreateHostServices();
        });
    if(launch >= 0)
        return launch;

    return krkr::sdl::Run(krkr::launch::DesktopTarget());
}
