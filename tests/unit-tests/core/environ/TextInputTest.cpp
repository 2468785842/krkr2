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
