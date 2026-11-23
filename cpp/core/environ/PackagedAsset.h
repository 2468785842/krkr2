#pragma once

#include <cstddef>
#include <functional>
#include <string>

// Uses the host's file search rules for both local and packaged file names.
// Directories do not count as files.
bool TVPHostFileExists(const std::string &name);

// Calls the reader synchronously. The bytes are valid only during the call.
bool TVPReadPackagedAsset(
    const std::string &name,
    const std::function<void(const unsigned char *, std::size_t)> &reader);
