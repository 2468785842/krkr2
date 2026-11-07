#include <catch2/catch_test_macros.hpp>
#include "PointerCoordinates.h"

#include <cmath>
#include <limits>
#include <stdexcept>

TEST_CASE("pointer origin conversion uses continuous layer height", "[input]") {
    using namespace krkr::input;
    auto top = flipPointerY({ 12.5f, 480.f }, 480.f);
    REQUIRE(top.x == 12.5f);
    REQUIRE(top.y == 0.f);
    REQUIRE(flipPointerY({ 0.f, 0.f }, 480.f).y == 480.f);
    auto original = flipPointerY(top, 480.f);
    REQUIRE(original.x == 12.5f);
    REQUIRE(original.y == 480.f);
    REQUIRE(flipPointerY({ 0.f, 500.5f }, 480.f).y == -20.5f);
}

TEST_CASE("pointer pixels truncate toward zero without clipping", "[input]") {
    using namespace krkr::input;
    auto point = pointerPixel({ -1.75f, 500.75f });
    REQUIRE(point.x == -1);
    REQUIRE(point.y == 500);
    point = pointerPixel({ -.75f, -1.75f });
    REQUIRE(point.x == 0);
    REQUIRE(point.y == -1);
    point = pointerPixel(flipPointerY({ 15.75f, .75f }, 480.25f));
    REQUIRE(point.x == 15);
    REQUIRE(point.y == 479);
}

TEST_CASE("pointer integer conversion rejects undefined casts", "[input]") {
    using namespace krkr::input;
    const float minimum = std::numeric_limits<std::int32_t>::min();
    const float maximum =
        static_cast<float>(std::numeric_limits<std::int32_t>::max());
    REQUIRE(pointerPixel({ minimum, 0 }).x ==
            std::numeric_limits<std::int32_t>::min());
    REQUIRE(pointerPixel({ std::nextafter(maximum, 0.f), 0 }).x == 2147483520);
    for(float invalid :
        { maximum, std::nextafter(minimum, -INFINITY),
          std::numeric_limits<float>::quiet_NaN(), INFINITY, -INFINITY }) {
        REQUIRE_THROWS_AS(pointerPixel({ invalid, 0 }), std::out_of_range);
        REQUIRE_THROWS_AS(pointerPixel({ 0, invalid }), std::out_of_range);
    }
}
