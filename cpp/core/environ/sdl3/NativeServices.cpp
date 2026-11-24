#include "tjsCommHead.h"
#include "SdlHost.h"
#include "ConsoleOutput.h"
#include "Platform.h"
#include "TVPLog.h"
#include <SDL3/SDL.h>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <vector>
#if defined(_WIN32)
#include <windows.h>
#include <shlobj.h>
void TVPConsoleLog(const ttstr &, bool);
#endif

namespace krkr::sdl {
    std::unique_ptr<krkr::host::LocalServices> CreateHostServices() {
        std::filesystem::path executable;
        std::string writable;
#if defined(_WIN32)
        std::vector<wchar_t> buffer(32768);
        const auto size = GetModuleFileNameW(nullptr, buffer.data(),
                                             static_cast<DWORD>(buffer.size()));
        if(!size || size >= buffer.size())
            throw std::runtime_error("Cannot resolve executable path");
        executable = std::filesystem::path(buffer.data());
        PWSTR local = nullptr;
        if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE,
                                       nullptr, &local)))
            throw std::runtime_error("Cannot resolve LocalAppData");
        const std::filesystem::path root(local);
        CoTaskMemFree(local);
        writable = (root / executable.stem()).generic_u8string();
        krkr::host::SetConsoleListener(TVPConsoleLog);
#else
        executable = std::filesystem::read_symlink("/proc/self/exe");
        const auto environment =
            [](const char *key) -> std::optional<std::string> {
            const auto *value = std::getenv(key);
            return value ? std::optional<std::string>(value) : std::nullopt;
        };
        writable = krkr::config::LinuxWritableDirectory(
            executable.u8string(), environment("XDG_CONFIG_HOME"),
            environment("HOME"));
#endif
        return std::make_unique<krkr::host::LocalServices>(
            executable.parent_path() / "Resources", writable,
            ReportPreferenceWriteFailure);
    }
} // namespace krkr::sdl

#if KRKR2_SDL3_ONLY
void TVPOpenPatchLibUrl() {
    if(!SDL_OpenURL("https://zeas2.github.io/Kirikiroid2_patch/patch"))
        G_CoreLog.error("Unable to open patch library: {}", SDL_GetError());
}

iWindowLayer *TVPCreateAndAddWindow(tTJSNI_Window *owner) {
    return krkr::sdl::CreateGameWindow(owner);
}

bool TVPGetScreenSize(tjs_int index, tjs_int &width, tjs_int &height) {
    return index == 0 && krkr::sdl::GetScreenSize(width, height);
}

namespace TJS {
    void TVPConsoleLog(const ttstr &line) {
        G_Tjs2Log.info(line.AsStdString());
    }
} // namespace TJS

namespace krkr::video {
    std::unique_ptr<Presentation>
    CreateWindowPresentation(iWindowLayer &window, FrameCallback callback) {
        return sdl::CreateVideoPresentation(window, std::move(callback));
    }
} // namespace krkr::video

int TVPDrawSceneOnce(int interval) { return krkr::sdl::DrawOnce(interval); }
void TVPCaptureHostFrame(const std::string &path,
                         std::function<void(bool)> completion) {
    krkr::sdl::Capture(path, std::move(completion));
}

bool TVPCopyFile(const std::string &from, const std::string &to) {
    const auto source = std::filesystem::u8path(from);
    const auto destination = std::filesystem::u8path(to);
    std::error_code error;
    const auto sourcePath = std::filesystem::weakly_canonical(source, error);
    if(error)
        return false;
    const auto destinationPath =
        std::filesystem::weakly_canonical(destination, error);
    if(error)
        return false;
    auto src = sourcePath.begin(), dst = destinationPath.begin();
    for(;
        src != sourcePath.end() && dst != destinationPath.end() && *src == *dst;
        ++src, ++dst) {
    }
    if(src == sourcePath.end())
        return false;
    std::filesystem::copy(source, destination,
                          std::filesystem::copy_options::recursive |
                              std::filesystem::copy_options::overwrite_existing,
                          error);
    return !error;
}
#endif
