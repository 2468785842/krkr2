#pragma once

// Notify the engine on its main thread, before the backend stops/starts its
// animation loop. These do not own the backend's event pump or graphics
// context.
void TVPOnHostForeground();
void TVPOnHostBackground();
