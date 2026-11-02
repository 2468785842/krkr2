#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace krkr::launch {
    struct EngineOption {
        std::string name;
        std::string value;
    };

    struct LaunchOptions {
        std::optional<std::filesystem::path> game;
        std::vector<EngineOption> engineOptions;
        bool help = false;
    };

    // Arguments exclude argv[0], and use UTF-8 on all desktop platforms.
    LaunchOptions Parse(const std::vector<std::string> &arguments);
    // Prefer loose startup.tjs, then data.xp3. Archive contents are validated
    // later by the runtime's existing readers, before creating a GL window.
    std::filesystem::path ResolveTarget(const std::filesystem::path &input);
    const char *Usage(bool managementUI = true);
} // namespace krkr::launch
