#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string>
#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/syslog_sink.h>

#include "environ/cocos2d/AppDelegate.h"

int main(int argc, char **argv) {
    spdlog::set_level(spdlog::level::debug);

    static auto core_logger = spdlog::syslog_logger_mt("core", "core");
    static auto tjs2_logger = spdlog::syslog_logger_mt("tjs2", "tjs2");
    static auto plugin_logger = spdlog::syslog_logger_mt("plugin", "plugin");

    spdlog::set_default_logger(core_logger);

    static std::unique_ptr<TVPAppDelegate> pAppDelegate =
        std::make_unique<TVPAppDelegate>();
    return pAppDelegate->run();
}