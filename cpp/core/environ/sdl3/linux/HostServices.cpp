#include "tjsCommHead.h"
#include "SdlHost.h"
#include <filesystem>
#include <cstdlib>
#include <optional>

namespace krkr::sdl {
    std::unique_ptr<krkr::host::LocalServices> CreateHostServices() {
        std::filesystem::path executable;
        std::string writable;
        executable = std::filesystem::read_symlink("/proc/self/exe");
        const auto environment =
            [](const char *key) -> std::optional<std::string> {
            const auto *value = std::getenv(key);
            return value ? std::optional<std::string>(value) : std::nullopt;
        };
        writable = krkr::config::LinuxWritableDirectory(
            executable.u8string(), environment("XDG_CONFIG_HOME"),
            environment("HOME"));
        return std::make_unique<krkr::host::LocalServices>(
            executable.parent_path() / "Resources", writable,
            ReportPreferenceWriteFailure);
    }
} // namespace krkr::sdl
