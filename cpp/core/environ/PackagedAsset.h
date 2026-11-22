#pragma once

#include <cstddef>
#include <functional>
#include <string>

// Calls the reader synchronously. The bytes are valid only during the call.
bool TVPReadPackagedAsset(
    const std::string &name,
    const std::function<void(const unsigned char *, std::size_t)> &reader);
