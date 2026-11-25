#include "launch/LaunchOptions.h"

#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <fstream>
#include <stdexcept>

using namespace krkr::launch;
namespace fs = std::filesystem;

namespace {
    struct Project {
        fs::path root = fs::temp_directory_path() /
            ("krkr2-launch-" +
             std::to_string(
                 std::chrono::steady_clock::now().time_since_epoch().count()));
        Project() {
            if(!fs::create_directory(root))
                throw std::runtime_error(
                    "Cannot create isolated launch fixture");
        }
        ~Project() {
            std::error_code error;
            fs::remove_all(root, error);
        }
        void file(const fs::path &name) {
            std::ofstream(root / name) << "fixture";
        }
    };
} // namespace

TEST_CASE("desktop launch accepts explicit and legacy UTF-8 paths",
          "[launch]") {
    const std::string path = "游戏 folder/data.xp3";
    for(const auto &args :
        std::vector<std::vector<std::string>>{ { path },
                                               { "--game", path },
                                               { "--game=" + path },
                                               { "--xp3", path },
                                               { "--xp3=" + path } }) {
        auto options = Parse(args);
        REQUIRE(options.game->u8string() == path);
        REQUIRE_FALSE(options.help);
    }
    REQUIRE_FALSE(Parse({}).game);
    REQUIRE(Parse({ "--help" }).help);
    REQUIRE(Parse({ "-h" }).help);
}

TEST_CASE("desktop launch rejects ambiguous and malformed arguments",
          "[launch]") {
    for(const auto &args :
        std::vector<std::vector<std::string>>{ { "--game" },
                                               { "--game", "--help" },
                                               { "--game=" },
                                               { "" },
                                               { "a", "b" },
                                               { "a", "--game", "b" },
                                               { "--config", "a" },
                                               { "--unknown" },
                                               { "-" },
                                               { "--", "path" },
                                               { "--", "--game" },
                                               { "--", "-=x" },
                                               { "--loglevel" } })
        REQUIRE_THROWS_AS(Parse(args), std::invalid_argument);
}

TEST_CASE("desktop engine options preserve values and order", "[launch]") {
    auto options = Parse({ "-loglevel", "warn", "--game", "game", "--",
                           "-debug", "-name=a=b", "-empty=", "-debug=no" });
    REQUIRE(options.engineOptions.size() == 5);
    REQUIRE(options.engineOptions[0].name == "-loglevel");
    REQUIRE(options.engineOptions[0].value == "warn");
    REQUIRE(options.engineOptions[1].value == "yes");
    REQUIRE(options.engineOptions[2].value == "a=b");
    REQUIRE(options.engineOptions[3].value.empty());
    REQUIRE(options.engineOptions[4].value == "no");
    REQUIRE(Parse({ "--loglevel=error" }).engineOptions[0].value == "error");
}

TEST_CASE("desktop launch selects SDL3 explicitly", "[launch]") {
#if KRKR2_SDL3_ONLY
    REQUIRE(Parse({ "--game", "demo" }).host == DesktopHost::SDL3);
#else
    REQUIRE(Parse({ "--game", "demo" }).host == DesktopHost::Cocos);
#endif
    REQUIRE(Parse({ "--host=cocos", "demo" }).host == DesktopHost::Cocos);
    const auto sdl = Parse({ "--game", "demo", "--host=sdl3" });
    REQUIRE(sdl.host == DesktopHost::SDL3);
    REQUIRE(sdl.game->u8string() == "demo");
    REQUIRE(sdl.engineOptions.empty());
    REQUIRE_THROWS_AS(Parse({ "--host=unknown", "demo" }),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(Parse({ "--host=cocos", "--host=sdl3", "demo" }),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(Parse({ "demo", "--", "--host=sdl3" }),
                      std::invalid_argument);
}

TEST_CASE("desktop settings are validated separately from script arguments",
          "[launch][preferences]") {
    const auto options = Parse(
        { "--host=cocos", "--set", "renderer=software", "--set=renderer=opengl",
          "--set", "default_font=字体=a.ttf", "--set=showfps=1",
          "--set=menu_handler_opa=0.25", "game", "--", "-renderer=legacy" });
    REQUIRE(options.settings.at("renderer") == "opengl");
    REQUIRE(options.settings.at("default_font") == "字体=a.ttf");
    REQUIRE(options.settings.at("menu_handler_opa") == "0.25");
    REQUIRE(options.engineOptions.size() == 1);
    REQUIRE(options.engineOptions.front().value == "legacy");
    REQUIRE(Parse({ "--list-settings" }).listSettings);
    REQUIRE(
        Parse({ "--set=default_font=" }).settings.at("default_font").empty());

    for(const char *invalid :
        { "", "renderer", "unknown=1", "renderer=vulkan", "showfps=2",
          "fps_limit=0", "fps_limit=999999999999", "menu_handler_opa=1.1",
          "menu_handler_opa=0,5" })
        REQUIRE_THROWS_AS(Parse({ "--set", invalid }), std::invalid_argument);
    REQUIRE_THROWS_AS(Parse({ "--host=sdl3", "--set=renderer=opengl" }),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(Parse({ "--", "--set=showfps=1" }),
                      std::invalid_argument);
}

TEST_CASE("desktop directory launch prefers loose startup then data archive",
          "[launch]") {
    Project project;
    REQUIRE_THROWS_AS(ResolveTarget(project.root), std::invalid_argument);
    project.file("data.xp3");
    REQUIRE(ResolveTarget(project.root) == project.root / "data.xp3");
    project.file("startup.tjs");
    REQUIRE(fs::equivalent(ResolveTarget(project.root), project.root));
    REQUIRE(ResolveTarget(project.root).generic_u8string().back() == '/');
    REQUIRE(ResolveTarget(project.root / "data.xp3") ==
            project.root / "data.xp3");
    REQUIRE_THROWS_AS(ResolveTarget(project.root / "missing"),
                      std::invalid_argument);
    REQUIRE_THROWS_AS(ResolveTarget({}), std::invalid_argument);
}

TEST_CASE("desktop resolution handles Unicode and rejects directories named "
          "like scripts",
          "[launch]") {
    Project project;
    const auto game = project.root / fs::u8path("测试 游戏");
    fs::create_directory(game);
    fs::create_directory(game / "startup.tjs");
    fs::create_directory(game / "data.xp3");
    REQUIRE_THROWS_AS(ResolveTarget(game), std::invalid_argument);
    const auto archive = game / fs::u8path("测试.xp3");
    std::ofstream(archive) << "archive content is validated by runtime";
    REQUIRE(ResolveTarget(archive) == archive);
    REQUIRE(ResolveTarget(archive.parent_path() / "." / archive.filename()) ==
            archive);
}

TEST_CASE(
    "desktop native host default validates renderer and explicit overrides",
    "[launch]") {
    REQUIRE(Parse({}, DesktopHost::SDL3).host == DesktopHost::SDL3);
    REQUIRE(Parse({ "--host=sdl3" }, DesktopHost::SDL3).host ==
            DesktopHost::SDL3);
    REQUIRE(Parse({ "--host=cocos" }, DesktopHost::SDL3).host ==
            DesktopHost::Cocos);
    REQUIRE_THROWS_AS(Parse({ "--set", "renderer=opengl" }, DesktopHost::SDL3),
                      std::invalid_argument);
    REQUIRE_NOTHROW(Parse({ "--set", "renderer=software" }, DesktopHost::SDL3));
    REQUIRE_THROWS_AS(
        Parse({ "--host=sdl3", "--host=cocos" }, DesktopHost::SDL3),
        std::invalid_argument);
}
