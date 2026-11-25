#include <catch2/catch_test_macros.hpp>
#include "ConfigFormat.h"
#include <tinyxml2/tinyxml2.h>

#include <filesystem>
#include <fstream>
#include <iterator>

TEST_CASE("config XML matches legacy parser for shipped locale files") {
    std::size_t count = 0;
    for(const auto &file : std::filesystem::directory_iterator(
            std::filesystem::u8path(KRKR_LEGACY_LOCALE_DIR))) {
        if(file.path().extension() != ".xml")
            continue;
        CAPTURE(file.path().u8string());
        std::ifstream input(file.path(), std::ios::binary);
        REQUIRE(input.good());
        const std::string xml{ std::istreambuf_iterator<char>(input),
                               std::istreambuf_iterator<char>() };
        tinyxml2::XMLDocument legacy;
        REQUIRE(legacy.Parse(xml.c_str(), xml.size()) == tinyxml2::XML_SUCCESS);
        REQUIRE(legacy.RootElement());
        krkr::config::Values expected;
        for(auto *item = legacy.RootElement()->FirstChildElement(); item;
            item = item->NextSiblingElement()) {
            const auto *id = item->Attribute("id");
            const auto *text = item->Attribute("text");
            if(id && text)
                expected[id] = text;
        }
        REQUIRE_FALSE(expected.empty());
        const auto actual = krkr::config::ReadLegacyLocale(xml);
        REQUIRE(actual.size() == expected.size());
        for(const auto &[id, text] : expected) {
            CAPTURE(id);
            REQUIRE(actual.count(id) == 1);
            REQUIRE(actual.at(id) == text);
        }
        if(file.path().stem() == "en_us")
            REQUIRE(actual.at("about_content").find("\nReport bugs here:\n") !=
                    std::string::npos);
        ++count;
    }
    REQUIRE(count > 0);
}

TEST_CASE("config migrates legacy parser preferences to JSON") {
    krkr::config::Preferences settings;
    settings.values = { { "language", "中文<&\"\t\r\n" },
                        { "long", std::string(10000, 'x') } };
    settings.arguments = { { "arg", "first" }, { "arg", "second" } };
    settings.keyMap = { { -1, 2 }, { 65, 66 } };
    const std::string xml =
        "<GlobalPreference><Item key='language' "
        "value='中文&lt;&amp;&quot;&#9;&#13;&#10;'/><Item key='long' value='" +
        std::string(10000, 'x') +
        "'/><Custom key='arg' value='first'/><Custom key='arg' "
        "value='second'/><KeyMap key='-1' value='2'/><KeyMap key='65' "
        "value='66'/></GlobalPreference>";
    tinyxml2::XMLDocument legacy;
    REQUIRE(legacy.Parse(xml.c_str(), xml.size()) == tinyxml2::XML_SUCCESS);
    auto *root = legacy.RootElement();
    REQUIRE(root);
    krkr::config::Preferences restored;
    for(auto *item = root->FirstChildElement("Item"); item;
        item = item->NextSiblingElement("Item"))
        restored.values[item->Attribute("key")] = item->Attribute("value");
    for(auto *item = root->FirstChildElement("Custom"); item;
        item = item->NextSiblingElement("Custom"))
        restored.arguments.emplace_back(item->Attribute("key"),
                                        item->Attribute("value"));
    for(auto *item = root->FirstChildElement("KeyMap"); item;
        item = item->NextSiblingElement("KeyMap")) {
        int key = 0, value = 0;
        REQUIRE(item->QueryIntAttribute("key", &key) == tinyxml2::XML_SUCCESS);
        REQUIRE(item->QueryIntAttribute("value", &value) ==
                tinyxml2::XML_SUCCESS);
        restored.keyMap.emplace(key, value);
    }
    REQUIRE(restored.values == settings.values);
    REQUIRE(restored.arguments == settings.arguments);
    REQUIRE(restored.keyMap == settings.keyMap);
    const auto migrated = krkr::config::ReadLegacyPreferences(xml);
    REQUIRE(migrated);
    const auto roundtrip = krkr::config::ReadPreferences(
        krkr::config::WritePreferences(*migrated));
    REQUIRE(roundtrip);
    REQUIRE(roundtrip->values == restored.values);
    REQUIRE(roundtrip->arguments == restored.arguments);
    REQUIRE(roundtrip->keyMap == restored.keyMap);

    const std::string raw =
        "<GlobalPreference><Custom key='lines' value='a\tb\r\nc\n\rd\re'/>"
        "</GlobalPreference>";
    tinyxml2::XMLDocument oldWhitespace;
    REQUIRE(oldWhitespace.Parse(raw.c_str(), raw.size()) ==
            tinyxml2::XML_SUCCESS);
    const auto parsed = krkr::config::ReadLegacyPreferences(raw);
    REQUIRE(parsed);
    REQUIRE(
        parsed->arguments.at(0).second ==
        oldWhitespace.RootElement()->FirstChildElement()->Attribute("value"));
}
