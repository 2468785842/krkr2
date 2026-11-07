#include "PostUpdateEvent.h"

// Keep registration separate from invocation to catch per-TU hook storage.
void RegisterPostUpdateFromAnotherUnit(void (*callback)()) {
    TVPSetPostUpdateEvent(callback);
}
