#include <catch2/catch_test_macros.hpp>
#include "ConfigXml.h"
#include <tinyxml2/tinyxml2.h>

#include <filesystem>
#include <fstream>
#include <iterator>

TEST_CASE("config XML matches legacy parser for shipped locale files") {
    std::size_t count = 0;
    for(const auto &file : std::filesystem::directory_iterator(
            std::filesystem::u8path(KRKR_ASSET_DIR) / "locale")) {
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
        const auto actual = krkr::config::ReadLocale(xml);
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

TEST_CASE(
    "config XML serialized preferences remain readable by legacy parser") {
    krkr::config::Preferences settings;
    settings.values = { { "language", "中文<&\"\t\r\n" },
                        { "long", std::string(10000, 'x') } };
    settings.arguments = { { "arg", "first" }, { "arg", "second" } };
    settings.keyMap = { { -1, 2 }, { 65, 66 } };
    const auto xml = krkr::config::WritePreferences(settings);
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

    const std::string raw =
        "<GlobalPreference><Custom key='lines' value='a\tb\r\nc\n\rd\re'/>"
        "</GlobalPreference>";
    tinyxml2::XMLDocument oldWhitespace;
    REQUIRE(oldWhitespace.Parse(raw.c_str(), raw.size()) ==
            tinyxml2::XML_SUCCESS);
    const auto parsed = krkr::config::ReadPreferences(raw);
    REQUIRE(parsed);
    REQUIRE(
        parsed->arguments.at(0).second ==
        oldWhitespace.RootElement()->FirstChildElement()->Attribute("value"));
}
