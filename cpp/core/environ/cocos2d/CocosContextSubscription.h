#pragma once

#include "GLHost.h"
#include "base/CCEventDispatcher.h"
#include "base/CCEventListenerCustom.h"
#include "base/CCEventType.h"
#include <new>
#include <stdexcept>
#include <utility>

namespace krkr::cocosrender {
    class ContextSubscription final : public glhost::ContextSubscription {
        cocos2d::EventDispatcher &dispatcher;
        cocos2d::EventListenerCustom *listener;

    public:
        ContextSubscription(cocos2d::EventDispatcher &dispatcher,
                            std::function<void()> callback) :
            dispatcher(dispatcher) {
            if(!callback)
                throw std::invalid_argument("Context callback is empty");
            listener = cocos2d::EventListenerCustom::create(
                EVENT_RENDERER_RECREATED,
                [callback = std::move(callback)](cocos2d::EventCustom *) {
                    callback();
                });
            if(!listener)
                throw std::bad_alloc();
            listener->retain();
            dispatcher.retain();
            try {
                dispatcher.addEventListenerWithFixedPriority(listener, 1);
            } catch(...) {
                dispatcher.removeEventListener(listener);
                listener->release();
                dispatcher.release();
                throw;
            }
        }

        ~ContextSubscription() override {
            dispatcher.removeEventListener(listener);
            listener->release();
            dispatcher.release();
        }
        ContextSubscription(const ContextSubscription &) = delete;
        ContextSubscription &operator=(const ContextSubscription &) = delete;
    };
} // namespace krkr::cocosrender
