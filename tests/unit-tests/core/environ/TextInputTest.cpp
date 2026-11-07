#include <catch2/catch_test_macros.hpp>
#include "TextInput.h"

TEST_CASE("committed text decodes ASCII and BMP characters", "[input][text]") {
    REQUIRE(krkr::input::decodeTextInput("") == std::u16string{});
    REQUIRE(krkr::input::decodeTextInput("A\r\n\t") == u"A\r\n\t");
    REQUIRE(krkr::input::decodeTextInput("\xc3\xa9\xe4\xb8\xad\xe3\x81\x82") ==
            u"\u00e9\u4e2d\u3042");
    REQUIRE(krkr::input::decodeTextInput("e\xcc\x81") == u"e\u0301");
}

TEST_CASE("committed text emits UTF16 surrogate pairs", "[input][text]") {
    REQUIRE(krkr::input::decodeTextInput("\xf0\x90\x80\x80") == u"\U00010000");
    REQUIRE(krkr::input::decodeTextInput("\xf0\x9f\x99\x82") == u"\U0001f642");
    REQUIRE(krkr::input::decodeTextInput("\xf4\x8f\xbf\xbf") == u"\U0010ffff");
    REQUIRE(
        krkr::input::decodeTextInput("\xed\x9f\xbf\xee\x80\x80\xef\xbf\xbf") ==
        u"\ud7ff\ue000\uffff");
}

TEST_CASE("committed text preserves NUL and concatenated commits",
          "[input][text]") {
    REQUIRE(krkr::input::decodeTextInput(std::string("a\0b", 3)) ==
            std::u16string(u"a\0b", 3));
    REQUIRE(krkr::input::decodeTextInput("A\xe4\xb8\xad\xf0\x9f\x99\x82") ==
            u"A\u4e2d\U0001f642");
}

TEST_CASE("committed text rejects malformed UTF8 without a prefix",
          "[input][text]") {
    for(const std::string invalid :
        { "\x80", "\xc0\x80", "\xc1\xbf", "\xe0\x80\x80", "\xf0\x80\x80\x80",
          "\xed\xa0\x80", "\xed\xbf\xbf", "\xf4\x90\x80\x80",
          "\xf5\x80\x80\x80", "\xff", "\xc2\x20", "\xe4\xb8\x41",
          "\xf0\x9f\x41\x80" }) {
        REQUIRE_FALSE(krkr::input::decodeTextInput(invalid));
        REQUIRE_FALSE(krkr::input::decodeTextInput("valid" + invalid));
    }
}

TEST_CASE("committed text rejects every truncated multibyte sequence",
          "[input][text]") {
    for(const std::string valid :
        { "\xc2\x80", "\xe4\xb8\xad", "\xf0\x9f\x99\x82" })
        for(std::size_t length = 1; length < valid.size(); ++length)
            REQUIRE_FALSE(
                krkr::input::decodeTextInput(valid.substr(0, length)));
}

TEST_CASE("committed UTF16 encodes BMP and embedded NUL as standard UTF8",
          "[input][text]") {
    REQUIRE(krkr::input::encodeTextInput(u"") == std::string{});
    REQUIRE(krkr::input::encodeTextInput(u"A\u4e2d\u3042\u00e9e\u0301") ==
            "A\xe4\xb8\xad\xe3\x81\x82\xc3\xa9\x65\xcc\x81");
    REQUIRE(krkr::input::encodeTextInput(std::u16string_view(u"A\0B", 3)) ==
            std::string("A\0B", 3));
}

TEST_CASE("committed UTF16 encodes complete supplementary scalars",
          "[input][text]") {
    REQUIRE(krkr::input::encodeTextInput(u"\U00010000\U0001f642\U0010ffff") ==
            "\xf0\x90\x80\x80\xf0\x9f\x99\x82\xf4\x8f\xbf\xbf");
    REQUIRE(krkr::input::encodeTextInput(u"\ud7ff\ue000\uffff") ==
            "\xed\x9f\xbf\xee\x80\x80\xef\xbf\xbf");
}

TEST_CASE(
    "committed UTF16 rejects lone or mismatched surrogates without a prefix",
    "[input][text]") {
    for(unsigned unit = 0xd800; unit <= 0xdfff; ++unit) {
        const std::u16string lone(1, static_cast<char16_t>(unit));
        REQUIRE_FALSE(krkr::input::encodeTextInput(lone));
        REQUIRE_FALSE(krkr::input::encodeTextInput(u"valid" + lone));
        REQUIRE_FALSE(krkr::input::encodeTextInput(lone + u"A"));
    }
    REQUIRE_FALSE(
        krkr::input::encodeTextInput(std::u16string{ 0xd800, 0xd800 }));
    REQUIRE_FALSE(
        krkr::input::encodeTextInput(std::u16string{ 0xdc00, 0xd800 }));
}
