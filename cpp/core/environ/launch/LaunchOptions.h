#pragma once

#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace krkr::launch {
    struct EngineOption {
        std::string name;
        std::string value;
    };

    enum class DesktopHost { SDL3 };
    inline constexpr DesktopHost DefaultDesktopHost = DesktopHost::SDL3;

    struct LaunchOptions {
        std::optional<std::filesystem::path> game;
        std::vector<EngineOption> engineOptions;
        std::map<std::string, std::string> settings;
        DesktopHost host = DefaultDesktopHost;
        bool help = false;
        bool listSettings = false;
    };

    // Arguments exclude argv[0], and use UTF-8 on all desktop platforms.
    LaunchOptions Parse(const std::vector<std::string> &arguments,
                        DesktopHost defaultHost = DefaultDesktopHost);
    // Prefer loose startup.tjs, then data.xp3. Archive contents are validated
    // later by the runtime's existing readers, before creating a GL window.
    std::filesystem::path ResolveTarget(const std::filesystem::path &input);
    const char *Usage();
} // namespace krkr::launch
