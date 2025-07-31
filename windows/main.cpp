#include "main.h"
#include <cocos2d.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <shellapi.h>

#include "environ/cocos2d/AppDelegate.h"

#include "environ/ui/MainFileSelectorForm.h"
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
        // Convert std::wstring to UTF-8 std::string using WideCharToMultiByte
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, xp3Path.c_str(), (int)xp3Path.size(), NULL, 0, NULL, NULL);
        std::string xp3PathUtf8(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, xp3Path.c_str(), (int)xp3Path.size(), &xp3PathUtf8[0], size_needed, NULL, NULL);
        spdlog::info("XP3 文件路径: {}", xp3PathUtf8);
        TVPMainFileSelectorForm::filePath = xp3Path;
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