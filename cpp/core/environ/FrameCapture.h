#pragma once

#include <functional>
#include <string>

// Capture the final host frame, including UI, at the backend's frame boundary.
// Completion may be asynchronous; it reports whether the file was saved.
void TVPCaptureHostFrame(const std::string &path,
                         std::function<void(bool)> completion);
