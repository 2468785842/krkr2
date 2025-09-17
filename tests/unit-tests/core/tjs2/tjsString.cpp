//
// Created by LiDon on 2025/9/16.
//
#include <catch2/catch_test_macros.hpp>

#include "tjsString.h"

TEST_CASE("string to tjsString") {
    using namespace TJS;
    SECTION("Japanese convert") {
        auto v1 = "レイヤー 1";
        auto v1_utf8 = u8"レイヤー 1";
        // auto v1_cp932 = boost::locale::conv::from_utf(v1_utf8, "cp932");
        // auto v1_shift_jis = boost::locale::conv::from_utf(v1_utf8,
        // "SHIFT_JIS");
        auto v1_tjs = ttstr{ v1 };
        REQUIRE(v1_tjs == v1);
        REQUIRE(v1_tjs == v1_utf8);
        REQUIRE(v1_tjs == v1_tjs);
        REQUIRE(v1_tjs.AsStdString() == v1);
    }
}