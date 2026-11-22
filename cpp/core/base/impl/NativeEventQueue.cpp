#include "NativeEventQueue.h"
#include "MainThreadMessageQueue.h"

void NativeEventQueueImplement::PostEvent(const NativeEvent &ev) {
    krkr::event::MainThreadMessages().Post(
        [this, event = ev]() mutable { Dispatch(event); }, this, ev.Message);
}

void NativeEventQueueImplement::Clear(unsigned msg) {
    krkr::event::MainThreadMessages().Cancel(this, msg);
}
