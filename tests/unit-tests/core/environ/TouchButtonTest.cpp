#include <catch2/catch_test_macros.hpp>
#include "TouchButton.h"

void selectTouchContacts(krkr::input::TouchButton &gesture, std::size_t count);

TEST_CASE("touch buttons select by contacts before the actual press",
          "[input]") {
    krkr::input::TouchButton gesture;
    REQUIRE(gesture.selectForContacts(1));
    REQUIRE(gesture.value() == 1);
    REQUIRE(gesture.selectForContacts(2));
    REQUIRE(gesture.press() == 2);
    REQUIRE(gesture.finish() == 2);
    REQUIRE(gesture.value() == 1);
    REQUIRE(gesture.selectForContacts(3));
    REQUIRE(gesture.press() == 4);
    REQUIRE(gesture.finish() == 4);
    REQUIRE_FALSE(gesture.selectForContacts(0));
    REQUIRE_FALSE(gesture.selectForContacts(4));
}

TEST_CASE("touch buttons preserve the down button when fingers change",
          "[input]") {
    krkr::input::TouchButton gesture;
    REQUIRE(gesture.press() == 1);
    REQUIRE_FALSE(gesture.selectForContacts(2));
    REQUIRE_FALSE(gesture.selectForContacts(3));
    REQUIRE(gesture.value() == 1);
    REQUIRE_FALSE(gesture.press()); // A timer and a move cannot press twice.
    REQUIRE(gesture.finish() == 1);
    REQUIRE_FALSE(gesture.finish());
}

TEST_CASE("touch button cancellation discards a deferred or held gesture",
          "[input]") {
    krkr::input::TouchButton gesture;
    gesture.selectForContacts(2);
    REQUIRE_FALSE(gesture.finish()); // No fabricated up for an unpressed tap.
    gesture.selectForContacts(2);
    REQUIRE(gesture.press() == 2);
    gesture.reset(); // Focus/window cancellation already released shared VKs.
    REQUIRE_FALSE(gesture.finish()); // The old contact's late up is inert.
    REQUIRE_FALSE(gesture.pressed());
    REQUIRE(gesture.press() == 1); // A new gesture starts from left again.
}

TEST_CASE("touch button ownership crosses translation units with its instance",
          "[input]") {
    krkr::input::TouchButton firstWindow, secondWindow;
    selectTouchContacts(firstWindow, 2);
    selectTouchContacts(secondWindow, 3);
    REQUIRE(firstWindow.press() == 2);
    REQUIRE(secondWindow.press() == 4);
    firstWindow.reset();
    REQUIRE(secondWindow.finish() == 4);
    REQUIRE_FALSE(firstWindow.finish());
}
