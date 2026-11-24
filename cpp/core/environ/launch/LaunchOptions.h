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

    enum class DesktopHost { Cocos, SDL3 };
#if KRKR2_SDL3_ONLY
    inline constexpr DesktopHost DefaultDesktopHost = DesktopHost::SDL3;
#else
    inline constexpr DesktopHost DefaultDesktopHost = DesktopHost::Cocos;
#endif

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
