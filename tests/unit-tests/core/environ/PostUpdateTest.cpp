#include <catch2/catch_test_macros.hpp>
#include <stdexcept>
#include <string>
#include "FrameStep.h"
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

namespace {
    std::string steps;
    void runEngine() { steps += "engine "; }
    void recycleTextures() { steps += "recycle "; }
    void recordHook() { steps += "hook "; }
    void draw() { steps += "draw "; }
    void swapBuffers() { steps += "swap "; }
    void failedDraw() { throw std::runtime_error("draw failed"); }

    struct FrameScope {
        HookScope hook;

        FrameScope() {
            steps.clear();
            TVPSetPostUpdateEvent(recordHook);
        }
    };
} // namespace

TEST_CASE("normal frame runs engine and recycling before the hook",
          "[frame-hook]") {
    FrameScope scope;
    krkr::event::UpdateFrame(runEngine, recycleTextures);
    REQUIRE(steps == "engine recycle hook ");
}

TEST_CASE("modal frame waits until due and draws only one frame",
          "[frame-hook]") {
    FrameScope scope;
    krkr::event::ModalFramePacer pacer;
    REQUIRE(pacer.DrawIfDue(100, 30, draw, swapBuffers) == 30);
    REQUIRE(pacer.DrawIfDue(129, 30, draw, swapBuffers) == 1);
    REQUIRE(steps.empty());

    REQUIRE(pacer.DrawIfDue(130, 30, draw, swapBuffers) == 0);
    REQUIRE(pacer.DrawIfDue(200, 30, draw, swapBuffers) == 0);
    REQUIRE(pacer.DrawIfDue(200, 30, draw, swapBuffers) == 30);
    REQUIRE(steps == "hook draw swap hook draw swap ");
}

TEST_CASE("modal pacing handles wrapping and long intervals between draws",
          "[frame-hook]") {
    FrameScope scope;
    krkr::event::ModalFramePacer pacer;
    REQUIRE(pacer.DrawIfDue(UINT32_MAX - 9, 30, draw, swapBuffers) == 30);
    REQUIRE(pacer.DrawIfDue(19, 30, draw, swapBuffers) == 1);
    REQUIRE(pacer.DrawIfDue(20, 30, draw, swapBuffers) == 0);
    REQUIRE(pacer.DrawIfDue(0x80000020u, 30, draw, swapBuffers) == 0);
    REQUIRE(steps == "hook draw swap hook draw swap ");
}

TEST_CASE("forced draw starts the next modal interval at its own tick",
          "[frame-hook]") {
    FrameScope scope;
    krkr::event::ModalFramePacer pacer;
    REQUIRE(pacer.DrawIfDue(10, 30, draw, swapBuffers) == 30);
    REQUIRE(pacer.DrawIfDue(12, 0, draw, swapBuffers) == 0);
    REQUIRE(pacer.DrawIfDue(12, -1, draw, swapBuffers) == 0);
    REQUIRE(pacer.DrawIfDue(13, 30, draw, swapBuffers) == 29);
    REQUIRE(steps == "hook draw swap hook draw swap ");
}

TEST_CASE("failed modal drawing leaves the frame due for retry",
          "[frame-hook]") {
    FrameScope scope;
    krkr::event::ModalFramePacer pacer;
    REQUIRE(pacer.DrawIfDue(10, 30, draw, swapBuffers) == 30);
    REQUIRE_THROWS_AS(pacer.DrawIfDue(40, 30, failedDraw, swapBuffers),
                      std::runtime_error);
    REQUIRE(steps == "hook ");
    REQUIRE(pacer.DrawIfDue(40, 30, draw, swapBuffers) == 0);
    REQUIRE(steps == "hook hook draw swap ");
}
