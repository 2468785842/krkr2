//
// Created by lidong on 25-6-21.
//

#include <catch2/catch_session.hpp>

#include <spdlog/sinks/stdout_color_sinks.h>

int main( int argc, char* argv[] ) {

    static auto core_logger = spdlog::stdout_color_mt("core");
    static auto tjs2_logger = spdlog::stdout_color_mt("tjs2");
    static auto plugin_logger = spdlog::stdout_color_mt("plugin");

    int result = Catch::Session().run( argc, argv );

    return result;
}