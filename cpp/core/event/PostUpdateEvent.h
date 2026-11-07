#pragma once

// Engine-thread hook shared by normal updates and modal drawing.
// A null callback unregisters the hook. The callback has static lifetime.
void TVPSetPostUpdateEvent(void (*callback)());
void TVPInvokePostUpdateEvent();
