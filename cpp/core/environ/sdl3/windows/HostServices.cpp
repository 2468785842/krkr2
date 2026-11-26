#include "tjsCommHead.h"
#include "SdlHost.h"
#include <filesystem>
#include "ConsoleOutput.h"
#include <windows.h>
#include <shlobj.h>
#include <stdexcept>
#include <vector>

void TVPConsoleLog(const ttstr &, bool);

namespace krkr::sdl {
    std::unique_ptr<krkr::host::LocalServices> CreateHostServices() {
        std::filesystem::path executable;
        std::string writable;
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
        return std::make_unique<krkr::host::LocalServices>(
            executable.parent_path() / "Resources", writable,
            ReportPreferenceWriteFailure);
    }
} // namespace krkr::sdl
