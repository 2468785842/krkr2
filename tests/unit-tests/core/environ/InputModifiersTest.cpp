#include <catch2/catch_test_macros.hpp>
#include "InputModifiers.h"
#include "InputState.h"
#include <array>

TEST_CASE("input modifiers preserve all legacy keyboard and mouse combinations",
          "[input]") {
    // Windows VK and TJS shift-mask ABI, independent of the adapter constants.
    constexpr std::array<unsigned, 6> keys{ 0x10, 0x12, 0x11, 1, 2, 4 };
    constexpr std::array<unsigned, 6> flags{ 1, 2, 4, 8, 16, 0 };
    for(unsigned combination = 0; combination < 64; ++combination) {
        krkr::input::KeyState state;
        state.press(65); // Ordinary keys must not contribute shift flags.
        unsigned expected = 0;
        for(unsigned bit = 0; bit < keys.size(); ++bit) {
            if(combination & (1u << bit)) {
                state.press(keys[bit]);
                expected |= flags[bit];
            }
        }
        REQUIRE(krkr::input::legacyShiftFlags(state) == expected);
    }
}

TEST_CASE("input modifier snapshots do not consume pending presses",
          "[input]") {
    krkr::input::KeyState state;
    state.press(0x10);
    state.press(0x11);
    state.release(0x11);
    state.latchReleasedPress(1);
    REQUIRE(krkr::input::legacyShiftFlags(state) == 1);
    REQUIRE(krkr::input::legacyShiftFlags(state) == 1);
    REQUIRE(state.query(0x10, false));
    REQUIRE(state.query(0x11, false));
    REQUIRE(state.query(1, false));
    REQUIRE(krkr::input::legacyShiftFlags(state) == 1);
}

TEST_CASE("input focus release clears modifiers before release callbacks",
          "[input]") {
    krkr::input::KeyState state;
    for(auto key : { 0x10u, 0x11u, 0x12u, 1u, 2u, 4u })
        state.press(key);
    REQUIRE(krkr::input::legacyShiftFlags(state) == 31);
    for(auto key : state.releaseAll()) {
        REQUIRE(krkr::input::legacyShiftFlags(state) == 0);
        REQUIRE_FALSE(state.isDown(key));
        REQUIRE(state.query(key, false));
    }
    REQUIRE(krkr::input::legacyShiftFlags(state) == 0);
}
