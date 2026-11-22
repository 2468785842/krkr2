#pragma once

#include <string>

// Query on the original render thread with its current GL context.
bool TVPCheckGLExtension(const std::string &name);
std::string TVPGetOpenGLInfo();
