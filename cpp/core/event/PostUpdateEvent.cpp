#include "PostUpdateEvent.h"

namespace {
    void (*postUpdate)() = nullptr;
}

void TVPSetPostUpdateEvent(void (*callback)()) { postUpdate = callback; }

void TVPInvokePostUpdateEvent() {
    if(auto callback = postUpdate)
        callback();
}
