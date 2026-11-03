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

TEST_CASE("input focus loss releases all devices before callbacks", "[input]") {
    krkr::input::KeyState state;
    state.press(0x1b5); // Controller.
    state.press(65); // Keyboard.
    state.press(0x10); // Shift.
    state.press(1); // Mouse.
    const auto released = state.releaseAll();
    REQUIRE(released == std::vector<std::uint32_t>{ 1, 0x10, 65, 0x1b5 });
    for(auto key : released)
        REQUIRE_FALSE(state.isDown(key));
    REQUIRE(state.releaseAll().empty());
    REQUIRE_FALSE(state.release(65)); // A late device release is redundant.
    state.press(65); // A fresh press after focus returns works normally.
    REQUIRE(state.query(65, true));
}

TEST_CASE("input focus loss preserves unread presses and released clicks",
          "[input]") {
    krkr::input::KeyState state;
    state.press(65);
    state.press(66);
    REQUIRE(state.query(66, false));
    state.latchReleasedPress(1);
    REQUIRE(state.releaseAll() == std::vector<std::uint32_t>{ 65, 66 });
    REQUIRE(state.query(65, false));
    REQUIRE_FALSE(state.query(65, false));
    REQUIRE_FALSE(state.query(66, false));
    REQUIRE(state.query(1, false));
    REQUIRE_FALSE(state.query(1, false));
}

TEST_CASE("input focus loss covers the complete virtual-key range", "[input]") {
    krkr::input::KeyState state;
    for(std::uint32_t key = 0; key < 0x200; ++key)
        state.press(key);
    const auto released = state.releaseAll();
    REQUIRE(released.size() == 0x200);
    for(std::uint32_t key = 0; key < 0x200; ++key) {
        REQUIRE(released[key] == key);
        REQUIRE_FALSE(state.query(key, true));
        REQUIRE_FALSE(state.query(key, false));
    }
}
