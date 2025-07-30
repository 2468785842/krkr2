#include "main.h"
#include <cocos2d.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "environ/cocos2d/AppDelegate.h"

std::wstring filePath;
USING_NS_CC;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 处理命令行参数，获取拖拽的文件路径
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    std::wstring xp3Path;
    if (argc > 1) {
        // 第一个参数是exe路径，第二个参数是拖拽的文件路径
        xp3Path = argv[1];
        spdlog::info("XP3 文件路径: {}", std::wstring_convert<std::codecvt_utf8<wchar_t>>().to_bytes(xp3Path));
        filePath = xp3Path;
    }

    LocalFree(argv);

    spdlog::set_level(spdlog::level::debug);

    static auto core_logger = spdlog::stdout_color_mt("core");
    static auto tjs2_logger = spdlog::stdout_color_mt("tjs2");
    static auto plugin_logger = spdlog::stdout_color_mt("plugin");

    spdlog::set_default_logger(core_logger);

    static std::unique_ptr<TVPAppDelegate> pAppDelegate =
        std::make_unique<TVPAppDelegate>();
    return pAppDelegate->run();
}