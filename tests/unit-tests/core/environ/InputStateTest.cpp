#include <catch2/catch_test_macros.hpp>
#include "InputState.h"
#include <limits>

TEST_CASE("input state keeps a held key after its press is consumed",
          "[input]") {
    krkr::input::KeyState state;
    state.press(65);
    REQUIRE(state.isDown(65));
    REQUIRE(state.query(65, false));
    REQUIRE_FALSE(state.query(65, false));
    REQUIRE(state.isDown(65));
    REQUIRE(state.release(65));
    REQUIRE_FALSE(state.isDown(65));
    REQUIRE_FALSE(state.query(65, false));
}

TEST_CASE("input current query consumes the pending press", "[input]") {
    krkr::input::KeyState state;
    state.press(65);
    REQUIRE(state.query(65, true));
    REQUIRE_FALSE(state.query(65, false));
    REQUIRE(state.query(65, true));
    REQUIRE(state.release(65));
    REQUIRE_FALSE(state.query(65, true));
}

TEST_CASE("input release retains an unread press", "[input]") {
    krkr::input::KeyState state;
    state.press(0x1b5); // VK_PADLEFT, without a backend-specific header.
    REQUIRE(state.release(0x1b5));
    REQUIRE_FALSE(state.isDown(0x1b5));
    REQUIRE(state.query(0x1b5, false));
    REQUIRE_FALSE(state.query(0x1b5, false));
    REQUIRE_FALSE(state.release(0x1b5));
}

TEST_CASE("input released pointer press is reported once", "[input]") {
    krkr::input::KeyState state;
    state.latchReleasedPress(1);
    REQUIRE_FALSE(state.isDown(1));
    REQUIRE(state.query(1, false));
    REQUIRE_FALSE(state.query(1, false));
    state.latchReleasedPress(1);
    REQUIRE_FALSE(state.query(1, true));
    REQUIRE_FALSE(state.query(1, false));
}

TEST_CASE("input repeat rearms the press without releasing the key",
          "[input]") {
    krkr::input::KeyState state;
    state.press(65);
    REQUIRE(state.query(65, false));
    state.press(65);
    REQUIRE(state.isDown(65));
    REQUIRE(state.query(65, false));
}

TEST_CASE("input key range and independent states are preserved", "[input]") {
    krkr::input::KeyState first, second;
    first.press(0x1ff);
    REQUIRE(first.isDown(0x1ff));
    REQUIRE_FALSE(second.isDown(0x1ff));
    for(auto invalid : { 0x200u, std::numeric_limits<std::uint32_t>::max() }) {
        first.press(invalid);
        first.latchReleasedPress(invalid);
        REQUIRE_FALSE(first.isDown(invalid));
        REQUIRE_FALSE(first.release(invalid));
        REQUIRE_FALSE(first.query(invalid, false));
        REQUIRE_FALSE(first.query(invalid, true));
    }
    REQUIRE(first.query(0x1ff, false));
}
