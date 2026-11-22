#pragma once

// Engine-thread entry points for the host's normal and nested event pumps.
void TVPUpdateEngineFrame();
void TVPProcessEngineMessages();

// Modal and startup drawing share a pacing clock. The current graphics
// backend draws at most one frame and returns the remaining delay in ms.
int TVPDrawSceneOnce(int interval);
