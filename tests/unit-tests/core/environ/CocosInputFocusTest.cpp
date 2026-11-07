#include <catch2/catch_test_macros.hpp>
#include "cocos2d/CocosInputFocus.h"
#include "InputModifiers.h"
#include <cocos2d.h>
#include <memory>

namespace {
    struct FocusFixture {
        cocos2d::AutoreleasePool pool;
        cocos2d::EventDispatcher *dispatcher = new cocos2d::EventDispatcher;
        krkr::input::KeyState state;
        FocusFixture() { dispatcher->setEnabled(true); }
        ~FocusFixture() { dispatcher->release(); }
        void lose() {
            dispatcher->dispatchCustomEvent(
                cocos2d::GLViewImpl::EVENT_WINDOW_UNFOCUSED);
        }
        void gain() {
            dispatcher->dispatchCustomEvent(
                cocos2d::GLViewImpl::EVENT_WINDOW_FOCUSED);
        }
        std::unique_ptr<krkr::cocosinput::InputFocus>
        observe(krkr::cocosinput::InputFocus::Cancel cancel) {
            return std::make_unique<krkr::cocosinput::InputFocus>(
                *dispatcher, state, cocos2d::GLViewImpl::EVENT_WINDOW_FOCUSED,
                cocos2d::GLViewImpl::EVENT_WINDOW_UNFOCUSED, std::move(cancel));
        }
    };
} // namespace

TEST_CASE("cocos focus clears all devices before cancellation",
          "[input][cocos]") {
    FocusFixture fixture;
    for(auto key : { 1u, 2u, 4u, 16u, 17u, 65u, 0x1b5u })
        fixture.state.press(key);
    std::vector<std::uint32_t> observed;
    std::unique_ptr<krkr::cocosinput::InputFocus> focus;
    focus = fixture.observe([&](const auto &keys) {
        observed = keys;
        REQUIRE_FALSE(focus->acceptsInput());
        REQUIRE(krkr::input::legacyShiftFlags(fixture.state) == 0);
        for(auto key : keys) {
            REQUIRE_FALSE(fixture.state.isDown(key));
            REQUIRE(fixture.state.query(key, false));
        }
    });
    fixture.lose();
    REQUIRE(observed ==
            std::vector<std::uint32_t>{ 1, 2, 4, 16, 17, 65, 0x1b5 });
}

TEST_CASE("cocos focus recovery needs fresh input and duplicate loss is inert",
          "[input][cocos]") {
    FocusFixture fixture;
    unsigned cancellations = 0;
    auto focus = fixture.observe([&](const auto &) { ++cancellations; });
    fixture.state.press(65);
    fixture.lose();
    fixture.lose();
    REQUIRE(cancellations == 1);
    REQUIRE_FALSE(focus->acceptsInput());
    REQUIRE_FALSE(fixture.state.release(65)); // Late native up is redundant.
    fixture.gain();
    fixture.gain();
    REQUIRE(focus->acceptsInput());
    REQUIRE_FALSE(fixture.state.isDown(65));
    fixture.state.press(65);
    REQUIRE(fixture.state.release(65));
    fixture.lose();
    REQUIRE(cancellations == 2);
}

TEST_CASE("cocos focus cancels a deferred touch even without held keys",
          "[input][cocos]") {
    FocusFixture fixture;
    fixture.state.latchReleasedPress(1);
    unsigned cancellations = 0;
    auto focus = fixture.observe([&](const auto &keys) {
        REQUIRE(keys.empty());
        ++cancellations;
    });
    fixture.lose();
    REQUIRE(cancellations == 1);
    REQUIRE(fixture.state.query(1, false));
}

TEST_CASE("cocos focus destruction removes only its own subscriptions",
          "[input][cocos]") {
    FocusFixture fixture;
    unsigned cancellations = 0, otherNotifications = 0;
    fixture.dispatcher->addCustomEventListener(
        cocos2d::GLViewImpl::EVENT_WINDOW_UNFOCUSED,
        [&](cocos2d::EventCustom *) { ++otherNotifications; });
    const auto references = fixture.dispatcher->getReferenceCount();
    auto focus = fixture.observe([&](const auto &) { ++cancellations; });
    REQUIRE(fixture.dispatcher->getReferenceCount() == references + 1);
    focus.reset();
    REQUIRE(fixture.dispatcher->getReferenceCount() == references);
    fixture.state.press(65);
    fixture.lose();
    fixture.gain();
    REQUIRE(cancellations == 0);
    REQUIRE(otherNotifications == 1);
    REQUIRE(fixture.state.isDown(65));
}
