#include "main.h"
#include <cocos2d.h>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <shellapi.h>
#include <boost/locale.hpp>

#include "tjsString.h"
#include "environ/cocos2d/AppDelegate.h"

#include "environ/ui/MainFileSelectorForm.h"
USING_NS_CC;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPTSTR lpCmdLine, int nCmdShow) {
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // 处理命令行参数，获取拖拽的文件路径
    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    if(argc > 1) {
        std::wstring xp3Path = argv[1];
        std::string xp3PathUtf8 =
            boost::locale::conv::utf_to_utf<char>(xp3Path);
        spdlog::info("XP3 文件路径: {}", xp3PathUtf8);
        TVPMainFileSelectorForm::filePath = xp3PathUtf8;
    }

    LocalFree(argv);

    spdlog::set_level(spdlog::level::debug);

    static auto core_logger = spdlog::stdout_color_mt("core");
    static auto tjs2_logger = spdlog::stdout_color_mt("tjs2");
    static auto plugin_logger = spdlog::stdout_color_mt("plugin");

    spdlog::set_default_logger(core_logger);

    static auto pAppDelegate = std::make_unique<TVPAppDelegate>();
    return pAppDelegate->run();
}