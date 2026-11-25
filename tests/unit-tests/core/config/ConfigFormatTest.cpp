#include <catch2/catch_test_macros.hpp>
#include "ConfigFormat.h"

#include <filesystem>
#include <fstream>
#include <iterator>

using namespace krkr::config;

TEST_CASE("config XML reads legacy values arguments and key maps") {
    const auto config = ReadLegacyPreferences(
        "\xEF\xBB\xBF<?xml version='1.0'?>"
        "<GlobalPreference><Item key='language' value='中文 &amp; 日本語'/>"
        "<Item key='renderer' value='opengl'/><Item key='renderer' "
        "value='software'/>"
        "<Item key='empty' value=''/><Item key='missing'/>"
        "<Custom key='arg' value='first'/><Custom key='arg' value='second'/>"
        "<KeyMap key='65' value='66'/><KeyMap key='65' value='67'/>"
        "<KeyMap key='0' value='66'/><KeyMap key='68' value='0'/>"
        "<KeyMap key='invalid' value='66'/><KeyMap key='999999999999999999999' "
        "value='66'/>"
        "<KeyMap key='-1' value='-2'/><Ignored key='ignored' value='ignored'/>"
        "</GlobalPreference>");
    REQUIRE(config);
    REQUIRE(config->values.at("language") == "中文 & 日本語");
    REQUIRE(config->values.at("renderer") == "software");
    REQUIRE(config->values.at("empty").empty());
    REQUIRE(config->values.count("missing") == 0);
    REQUIRE(config->arguments ==
            std::vector<std::pair<std::string, std::string>>{
                { "arg", "first" }, { "arg", "second" } });
    REQUIRE(config->keyMap == std::map<int, int>{ { -1, -2 }, { 65, 66 } });
}

TEST_CASE(
    "config JSON round trip preserves escaped whitespace and long values") {
    Preferences config;
    config.values = { { "special", "<&\"'\t\r\n中文" },
                      { "empty", "" },
                      { "long", std::string(10000, 'x') } };
    config.arguments = { { "duplicate", "1" }, { "duplicate", "2" } };
    config.keyMap = { { 1, 2 }, { -3, -4 } };
    const auto xml = WritePreferences(config);
    const auto parsed = ReadPreferences(xml);
    REQUIRE(parsed);
    REQUIRE(parsed->values == config.values);
    REQUIRE(parsed->arguments == config.arguments);
    REQUIRE(parsed->keyMap == config.keyMap);

    const auto legacy = ReadLegacyPreferences(
        "<GlobalPreference><Custom key='lines' value='a\tb\r\nc\n\rd\re'/>"
        "</GlobalPreference>");
    REQUIRE(legacy);
    REQUIRE(legacy->arguments.at(0).second == "a\tb\nc\nd\ne");
}

TEST_CASE("config XML ignores nested and unknown preference entries") {
    const auto parsed = ReadLegacyPreferences(
        "<LegacyRoot><!--comment--><Group><Item key='nested' "
        "value='x'/></Group>"
        "<Item key='known' value='y'/></LegacyRoot>");
    REQUIRE(parsed);
    REQUIRE(parsed->values == Values{ { "known", "y" } });
    const auto empty = ReadPreferences(WritePreferences({}));
    REQUIRE(empty);
    REQUIRE(empty->values.empty());
}

TEST_CASE("config XML rejects missing and malformed documents") {
    REQUIRE_FALSE(ReadLegacyPreferences(""));
    REQUIRE_FALSE(ReadLegacyPreferences("not XML"));
    REQUIRE_FALSE(
        ReadLegacyPreferences("<GlobalPreference><Item></GlobalPreference>"));
    REQUIRE(
        ReadLegacyLocale("<Locale><Item id='x' text='y'></Locale>").empty());
}

TEST_CASE(
    "config XML locale preserves duplicate empty and direct child rules") {
    const auto locale = ReadLegacyLocale(
        "<Locale><Text id='same' text='first'/><Text id='same' text='last'/>"
        "<!-- <Text id='comment' text='ignore'/> -->"
        "<![CDATA[<Text id='cdata' text='ignore'/>]]>"
        "<Text id='legacy' text='High Memory(<300MB)'/>"
        "<Text id='empty' text=''/><Text id='missing'/>"
        "<Group><Text id='nested' text='skip'/></Group></Locale>");
    REQUIRE(locale ==
            Values{ { "same", "last" },
                    { "empty", "" },
                    { "legacy", "High Memory(<300MB)" } });
}

TEST_CASE("config JSON rejects invalid schemas without partial settings") {
    for(const auto *text :
        { "", "[]", "null", "{", "{\"values\":{}}",
          R"({"version":2,"values":{}})", R"({"version":1,"values":{"x":1}})",
          R"({"version":1,"values":{},"arguments":[["x",1]]})",
          R"({"version":1,"values":{},"keyMap":[[1,9999999999999999999]]})",
          R"({"version":1,"values":{},"keyMap":[[1,-2147483649]]})",
          R"({"version":1,"values":{},"keyMap":[[1,2.0]]})" }) {
        CAPTURE(text);
        REQUIRE_FALSE(ReadPreferences(text));
    }
    REQUIRE(ReadLocale(R"({"ok":12})").empty());
    REQUIRE(ReadLocale("[]").empty());
    REQUIRE(ReadLocale(R"({"ok":"确定 & <OK>","empty":""})") ==
            Values{ { "ok", "确定 & <OK>" }, { "empty", "" } });
}

TEST_CASE("config JSON preserves duplicate arguments and first key mapping") {
    const auto parsed = ReadPreferences(R"({"version":1,"values":{},
        "arguments":[["arg","one"],["arg","two"]],
        "keyMap":[[65,66],[65,67],[0,1],[1,0],[-1,-2]]})");
    REQUIRE(parsed);
    REQUIRE(parsed->arguments ==
            std::vector<std::pair<std::string, std::string>>{
                { "arg", "one" }, { "arg", "two" } });
    REQUIRE(parsed->keyMap == std::map<int, int>{ { 65, 66 }, { -1, -2 } });
}

TEST_CASE("config shipped JSON locales preserve all legacy translations") {
    std::size_t count = 0;
    for(const auto &file :
        std::filesystem::directory_iterator(KRKR_LEGACY_LOCALE_DIR)) {
        std::ifstream old(file.path(), std::ios::binary);
        std::ifstream current(std::filesystem::path(KRKR_ASSET_DIR) / "locale" /
                                  (file.path().stem().string() + ".json"),
                              std::ios::binary);
        REQUIRE(old.good());
        REQUIRE(current.good());
        const std::string xml{ std::istreambuf_iterator<char>(old), {} };
        const std::string json{ std::istreambuf_iterator<char>(current), {} };
        CAPTURE(file.path().string());
        const auto expected = ReadLegacyLocale(xml);
        REQUIRE_FALSE(expected.empty());
        REQUIRE(ReadLocale(json) == expected);
        ++count;
    }
    REQUIRE(count == 2);
}
