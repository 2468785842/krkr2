#include <catch2/catch_test_macros.hpp>
#include "cocos2d/CocosContextSubscription.h"
#include <cocos2d.h>
#include <memory>
#include <type_traits>
#include <vector>

namespace {
    struct Fixture {
        cocos2d::AutoreleasePool pool;
        cocos2d::EventDispatcher *dispatcher = new cocos2d::EventDispatcher;
        Fixture() { dispatcher->setEnabled(true); }
        ~Fixture() { dispatcher->release(); }
        void notify() {
            dispatcher->dispatchCustomEvent(EVENT_RENDERER_RECREATED);
        }
    };
} // namespace

TEST_CASE("cocos context recreation preserves notifications and priority",
          "[cocos][context]") {
    Fixture fixture;
    std::vector<int> sequence;
    auto *other = cocos2d::EventListenerCustom::create(
        EVENT_RENDERER_RECREATED,
        [&](cocos2d::EventCustom *) { sequence.push_back(2); });
    fixture.dispatcher->addEventListenerWithFixedPriority(other, 2);
    krkr::cocosrender::ContextSubscription subscription(
        *fixture.dispatcher, [&] { sequence.push_back(1); });
    fixture.notify();
    fixture.notify();
    REQUIRE(sequence == std::vector<int>{ 1, 2, 1, 2 });
}

TEST_CASE("cocos context subscription destruction removes only its callback",
          "[cocos][context]") {
    Fixture fixture;
    int owned = 0, other = 0;
    auto *listener = cocos2d::EventListenerCustom::create(
        EVENT_RENDERER_RECREATED, [&](cocos2d::EventCustom *) { ++other; });
    fixture.dispatcher->addEventListenerWithFixedPriority(listener, 1);
    {
        krkr::cocosrender::ContextSubscription subscription(*fixture.dispatcher,
                                                            [&] { ++owned; });
        fixture.notify();
    }
    fixture.notify();
    REQUIRE(owned == 1);
    REQUIRE(other == 2);
}

TEST_CASE("cocos context subscription retains its dispatcher and listener",
          "[cocos][context]") {
    cocos2d::AutoreleasePool pool;
    auto *dispatcher = new cocos2d::EventDispatcher;
    dispatcher->setEnabled(true);
    int calls = 0;
    auto subscription =
        std::make_unique<krkr::cocosrender::ContextSubscription>(
            *dispatcher, [&] { ++calls; });
    REQUIRE(dispatcher->getReferenceCount() == 2);
    dispatcher->release();
    pool.clear(); // The registration must outlive autoreleased creation.
    dispatcher->dispatchCustomEvent(EVENT_RENDERER_RECREATED);
    REQUIRE(calls == 1);
    subscription.reset(); // Releases the last dispatcher reference safely.
}

TEST_CASE("cocos context subscription survives host-side listener removal",
          "[cocos][context]") {
    STATIC_REQUIRE(
        !std::is_copy_constructible_v<krkr::cocosrender::ContextSubscription>);
    Fixture fixture;
    int calls = 0;
    auto subscription =
        std::make_unique<krkr::cocosrender::ContextSubscription>(
            *fixture.dispatcher, [&] { ++calls; });
    fixture.notify();
    // Cocos preserves internal events in removeAllEventListeners(). The host
    // must explicitly remove the custom recreation event to exercise
    // detachment.
    fixture.dispatcher->removeCustomEventListeners(EVENT_RENDERER_RECREATED);
    fixture.pool.clear();
    fixture.notify();
    REQUIRE(calls == 1);
    subscription.reset();
    REQUIRE(fixture.dispatcher->getReferenceCount() == 1);
}
