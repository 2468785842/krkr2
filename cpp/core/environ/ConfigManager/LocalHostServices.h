#pragma once

#include "PreferenceStorage.h"
#include "resources/LocalAssetReader.h"

#include <filesystem>

namespace krkr::host {
    // Install before archive validation, and keep alive through engine cleanup.
    // No window system or graphics backend is needed to construct this scope.
    class LocalServices {
        assets::ScopedReader resources;
        config::ScopedPreferenceStorage preferences;

    public:
        LocalServices(const std::filesystem::path &resourceDirectory,
                      std::string writableDirectory,
                      std::function<void()> reportWriteFailure);
        LocalServices(const LocalServices &) = delete;
        LocalServices &operator=(const LocalServices &) = delete;
    };
} // namespace krkr::host
