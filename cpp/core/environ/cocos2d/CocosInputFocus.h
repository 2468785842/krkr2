#pragma once

#include "InputState.h"
#include "base/CCEventDispatcher.h"
#include "base/CCEventListenerCustom.h"

#include <functional>
#include <string>
#include <utility>

namespace krkr::cocosinput {
    // Desktop window focus is separate from AppDelegate background/foreground.
    // The scene owns this subscription, which retains the dispatcher. KeyState
    // must remain alive until the subscription is destroyed.
    class InputFocus final {
    public:
        using Cancel = std::function<void(const std::vector<std::uint32_t> &)>;

        InputFocus(cocos2d::EventDispatcher &dispatcher, input::KeyState &state,
                   const std::string &gained, const std::string &lost,
                   Cancel cancel) :
            dispatcher(dispatcher), state(state), cancel(std::move(cancel)) {
            dispatcher.retain();
            gainedListener = cocos2d::EventListenerCustom::create(
                gained, [this](cocos2d::EventCustom *) { focused = true; });
            lostListener = cocos2d::EventListenerCustom::create(
                lost, [this](cocos2d::EventCustom *) {
                    if(!focused)
                        return;
                    focused = false;
                    // Even an unpressed, deferred touch must be cancelled.
                    const auto released = this->state.releaseAll();
                    this->cancel(released);
                });
            dispatcher.addEventListenerWithFixedPriority(gainedListener, 1);
            dispatcher.addEventListenerWithFixedPriority(lostListener, 1);
        }

        ~InputFocus() {
            dispatcher.removeEventListener(gainedListener);
            dispatcher.removeEventListener(lostListener);
            dispatcher.release();
        }

        InputFocus(const InputFocus &) = delete;
        InputFocus &operator=(const InputFocus &) = delete;
        bool acceptsInput() const { return focused; }

    private:
        cocos2d::EventDispatcher &dispatcher;
        input::KeyState &state;
        Cancel cancel;
        cocos2d::EventListenerCustom *gainedListener, *lostListener;
        bool focused = true;
    };
} // namespace krkr::cocosinput
