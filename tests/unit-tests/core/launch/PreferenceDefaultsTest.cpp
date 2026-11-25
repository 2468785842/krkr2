#include "ConfigManager/PreferenceDefaults.h"

#include <catch2/catch_test_macros.hpp>
#include <map>
#include <variant>

namespace {
    struct Config {
        std::map<std::string, std::variant<bool, float, std::string>> values;
        bool IsValueExist(const std::string &key) {
            return values.count(key) != 0;
        }
        void SetValueInt(const std::string &key, int value) {
            values[key] = value != 0;
        }
        template <typename T>
        T GetValue(const std::string &key, const T &fallback) {
            return std::get<T>(values.try_emplace(key, fallback).first->second);
        }
    };
} // namespace

TEST_CASE("runtime preference defaults preserve persisted user choices",
          "[launch][preferences]") {
    Config config;
    config.values["renderer"] = std::string("opengl");
    config.values["default_font"] = std::string("custom.ttf");
    config.values["menu_handler_opa"] = 0.75f;
    config.values["GL_EXT_shader_framebuffer_fetch"] = true;
    TVPApplyRuntimePreferenceDefaults(config, true);
    REQUIRE(std::get<std::string>(config.values.at("renderer")) == "opengl");
    REQUIRE(std::get<std::string>(config.values.at("default_font")) ==
            "custom.ttf");
    REQUIRE(std::get<float>(config.values.at("menu_handler_opa")) == 0.75f);
    REQUIRE(
        std::get<bool>(config.values.at("GL_EXT_shader_framebuffer_fetch")));
}

TEST_CASE("Adreno workaround applies only to an unspecified setting",
          "[launch][preferences]") {
    Config config;
    TVPApplyRuntimePreferenceDefaults(config, true);
    REQUIRE_FALSE(
        std::get<bool>(config.values.at("GL_EXT_shader_framebuffer_fetch")));
    config.values["GL_EXT_shader_framebuffer_fetch"] = true;
    TVPApplyRuntimePreferenceDefaults(config, true);
    REQUIRE(
        std::get<bool>(config.values.at("GL_EXT_shader_framebuffer_fetch")));
}

TEST_CASE("runtime defaults initialize all modes and remain idempotent",
          "[launch][preferences]") {
    Config config;
    config.values["renderer"] = std::string("custom-renderer");
    TVPApplyRuntimePreferenceDefaults(config, false);
    REQUIRE(config.IsValueExist("software_draw_thread"));
    REQUIRE(config.IsValueExist("ogl_compress_tex"));
#ifdef TVP_PLATFORM_IOS
    REQUIRE(std::get<std::string>(config.values.at("memusage")) == "high");
    REQUIRE(
        std::get<bool>(config.values.at("GL_EXT_shader_framebuffer_fetch")));
#else
    REQUIRE(std::get<std::string>(config.values.at("memusage")) == "unlimited");
    REQUIRE_FALSE(
        std::get<bool>(config.values.at("GL_EXT_shader_framebuffer_fetch")));
#endif
#ifdef TVP_PLATFORM_ANDROID
    REQUIRE_FALSE(std::get<bool>(config.values.at("hide_android_sys_btn")));
#else
    REQUIRE_FALSE(config.IsValueExist("hide_android_sys_btn"));
#endif
    auto first = config.values;
    TVPApplyRuntimePreferenceDefaults(config, false);
    REQUIRE(config.values == first);
}
