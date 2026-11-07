#include <catch2/catch_test_macros.hpp>
#include "TextInput.h"
#include "base/ccUTF8.h"
#include <cstdint>

TEST_CASE("cocos text matches committed decoding for all Unicode scalars",
          "[input][cocos]") {
    std::u32string scalars;
    for(char32_t scalar = 0; scalar <= 0x10ffff; ++scalar)
        if(scalar < 0xd800 || scalar > 0xdfff)
            scalars.push_back(scalar);
    std::string utf8;
    REQUIRE(cocos2d::StringUtils::UTF32ToUTF8(scalars, utf8));
    std::u16string legacy;
    REQUIRE(cocos2d::StringUtils::UTF8ToUTF16(utf8, legacy));
    REQUIRE(krkr::input::decodeTextInput(utf8) == legacy);
    REQUIRE(krkr::input::encodeTextInput(legacy) == utf8);
}

TEST_CASE("cocos text matches committed decoding for arbitrary bytes",
          "[input][cocos]") {
    std::uint32_t random = 0x12345678;
    for(unsigned sample = 0; sample < 10000; ++sample) {
        std::string bytes;
        for(unsigned i = 0; i < sample % 9; ++i) {
            random = random * 1664525u + 1013904223u;
            bytes.push_back(static_cast<char>(random >> 24));
        }
        std::u16string legacy;
        const auto accepted = cocos2d::StringUtils::UTF8ToUTF16(bytes, legacy);
        const auto decoded = krkr::input::decodeTextInput(bytes);
        INFO("sample " << sample);
        REQUIRE(decoded.has_value() == accepted);
        if(decoded)
            REQUIRE(*decoded == legacy);
    }
}
