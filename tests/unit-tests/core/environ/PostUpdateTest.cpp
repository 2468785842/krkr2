#include <catch2/catch_test_macros.hpp>
#include "PostUpdateEvent.h"

void RegisterPostUpdateFromAnotherUnit(void (*callback)());

namespace {
    int firstCalls, secondCalls;
    void first() { ++firstCalls; }
    void second() { ++secondCalls; }
    void unregisterSelf() {
        ++firstCalls;
        TVPSetPostUpdateEvent(nullptr);
    }

    struct HookScope {
        HookScope() {
            firstCalls = secondCalls = 0;
            TVPSetPostUpdateEvent(nullptr);
        }
        ~HookScope() { TVPSetPostUpdateEvent(nullptr); }
    };
} // namespace

TEST_CASE("frame hook registration is visible across translation units",
          "[frame-hook]") {
    HookScope scope;
    RegisterPostUpdateFromAnotherUnit(first);
    TVPInvokePostUpdateEvent();
    TVPInvokePostUpdateEvent();
    REQUIRE(firstCalls == 2);
}

TEST_CASE("frame hook replacement and removal affect the next invocation",
          "[frame-hook]") {
    HookScope scope;
    RegisterPostUpdateFromAnotherUnit(first);
    TVPInvokePostUpdateEvent();
    RegisterPostUpdateFromAnotherUnit(second);
    TVPInvokePostUpdateEvent();
    TVPSetPostUpdateEvent(nullptr);
    TVPInvokePostUpdateEvent();
    REQUIRE(firstCalls == 1);
    REQUIRE(secondCalls == 1);
}

TEST_CASE("frame hook may unregister itself during invocation",
          "[frame-hook]") {
    HookScope scope;
    RegisterPostUpdateFromAnotherUnit(unregisterSelf);
    TVPInvokePostUpdateEvent();
    TVPInvokePostUpdateEvent();
    REQUIRE(firstCalls == 1);
}
