#include "tjsCommHead.h"
#include "SdlHost.h"
#include "ConfigManager/PreferenceStorage.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "WindowLayer.h"
#include "MenuItemImpl.h"
#include "TVPLog.h"
#include "MsgIntf.h"
#include "StorageImpl.h"
#include <SDL3/SDL.h>
#include <windows.h>
#include <shlobj.h>
#include <commdlg.h>
#include <algorithm>
#include <type_traits>
#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

namespace krkr::sdl {
    void InstallPreferences() {
        if(krkr::config::CurrentPreferenceStorage())
            return;
        PWSTR local = nullptr;
        if(FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE,
                                       nullptr, &local)))
            throw std::runtime_error("Cannot resolve LocalAppData");
        const std::filesystem::path root(local);
        CoTaskMemFree(local);
        // Preserve the legacy Windows directory: LocalAppData/<exe stem>.
        std::vector<wchar_t> executable(32768);
        const auto size = GetModuleFileNameW(
            nullptr, executable.data(), static_cast<DWORD>(executable.size()));
        if(!size || size >= executable.size())
            throw std::runtime_error("Cannot resolve executable path");
        auto directory = root / std::filesystem::path(executable.data()).stem();
        krkr::config::SetPreferenceStorage(
            std::make_shared<krkr::config::LocalPreferenceStorage>(
                directory.generic_u8string(), [] {
                    auto *locale = LocaleConfigManager::GetInstance();
                    SDL_ShowSimpleMessageBox(
                        SDL_MESSAGEBOX_ERROR,
                        locale->GetText("readonly_storage").c_str(),
                        locale->GetText("cannot_create_preference").c_str(),
                        nullptr);
                }));
    }
} // namespace krkr::sdl

#if KRKR2_SDL3_ONLY
void TVPOpenPatchLibUrl() {
    if(!SDL_OpenURL("https://zeas2.github.io/Kirikiroid2_patch/patch"))
        throw std::runtime_error(SDL_GetError());
}

iWindowLayer *TVPCreateAndAddWindow(tTJSNI_Window *owner) {
    return krkr::sdl::CreateGameWindow(owner);
}

bool TVPGetScreenSize(tjs_int index, tjs_int &width, tjs_int &height) {
    return index == 0 && krkr::sdl::GetScreenSize(width, height);
}

ttstr TVPGetPlatformName() { return TJS_W("Win32"); }
ttstr TVPGetOSName() { return TVPGetPlatformName(); }

void TVPConsoleLog(const ttstr &message, bool important) {
    // TVPAddLog already mirrors engine output through the logging sink.
    // Preserve a native debugger console without a second file-log entry.
    OutputDebugStringW((message.toWString() + L"\n").c_str());
    (void)important;
}
namespace TJS {
    void TVPConsoleLog(const ttstr &message) {
        G_Tjs2Log.info(message.AsStdString());
    }
} // namespace TJS

namespace krkr::video {
    std::unique_ptr<Presentation>
    CreateWindowPresentation(iWindowLayer &window, FrameCallback onFrame) {
        return krkr::sdl::CreateVideoPresentation(window, std::move(onFrame));
    }
} // namespace krkr::video

int TVPDrawSceneOnce(int interval) { return krkr::sdl::DrawOnce(interval); }
void TVPCaptureHostFrame(const std::string &path,
                         std::function<void(bool)> completion) {
    krkr::sdl::Capture(path, std::move(completion));
}

bool TVPCopyFile(const std::string &from, const std::string &to) {
    std::error_code error;
    std::filesystem::copy(std::filesystem::u8path(from),
                          std::filesystem::u8path(to),
                          std::filesystem::copy_options::recursive |
                              std::filesystem::copy_options::overwrite_existing,
                          error);
    return !error;
}

std::string TVPShowFileSelector(const std::string &title,
                                const std::string &filename,
                                std::string directory, bool save) {
    std::vector<wchar_t> path(32768);
    const auto initial = ttstr(filename).toWString();
    if(initial.size() >= path.size())
        throw std::runtime_error("File dialog path is too long");
    std::copy(initial.begin(), initial.end(), path.begin());
    const auto caption = ttstr(title).toWString();
    const auto folder =
        std::filesystem::u8path(directory).make_preferred().wstring();
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = GetActiveWindow();
    dialog.lpstrFile = path.data();
    dialog.nMaxFile = static_cast<DWORD>(path.size());
    dialog.lpstrTitle = caption.c_str();
    dialog.lpstrInitialDir = folder.empty() ? nullptr : folder.c_str();
    dialog.lpstrFilter = L"All files\0*.*\0\0";
    dialog.Flags = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST |
        (save ? OFN_OVERWRITEPROMPT : OFN_FILEMUSTEXIST);
    const bool accepted =
        save ? GetSaveFileNameW(&dialog) : GetOpenFileNameW(&dialog);
    if(!accepted) {
        if(const auto error = CommDlgExtendedError())
            throw std::runtime_error("File dialog failed: " +
                                     std::to_string(error));
        return {};
    }
    return std::filesystem::path(path.data()).generic_u8string();
}

void TVPShowPopMenu(tTJSNI_MenuItem *root) {
    struct MenuDeleter {
        void operator()(HMENU menu) const { DestroyMenu(menu); }
    };
    using Menu = std::unique_ptr<std::remove_pointer_t<HMENU>, MenuDeleter>;
    std::vector<tTJSNI_MenuItem *> actions;
    std::function<void(HMENU, tTJSNI_MenuItem *)> populate =
        [&](HMENU target, tTJSNI_MenuItem *parent) {
            for(auto *entry : parent->GetChildren()) {
                auto *item = static_cast<tTJSNI_MenuItem *>(entry);
                if(!item->GetVisible())
                    continue;
                ttstr caption;
                item->GetCaption(caption);
                if(caption.IsEmpty() || caption == TJS_W("+"))
                    continue;
                UINT flags = item->GetEnabled() ? MF_ENABLED : MF_GRAYED;
                if(item->GetChecked())
                    flags |= MF_CHECKED;
                if(caption == TJS_W("-")) {
                    AppendMenuW(target, MF_SEPARATOR, 0, nullptr);
                } else if(!item->GetChildren().empty()) {
                    Menu submenu(CreatePopupMenu());
                    populate(submenu.get(), item);
                    if(!AppendMenuW(target, flags | MF_POPUP,
                                    reinterpret_cast<UINT_PTR>(submenu.get()),
                                    caption.toWString().c_str()))
                        throw std::runtime_error("Cannot append popup menu");
                    submenu.release();
                } else {
                    actions.push_back(item);
                    if(!AppendMenuW(target, flags | MF_STRING, actions.size(),
                                    caption.toWString().c_str()))
                        throw std::runtime_error("Cannot append menu item");
                }
            }
        };
    Menu menu(CreatePopupMenu());
    if(!menu)
        throw std::runtime_error("Cannot create popup menu");
    populate(menu.get(), root);
    POINT point{};
    GetCursorPos(&point);
    const auto selected =
        TrackPopupMenuEx(menu.get(), TPM_RETURNCMD | TPM_NONOTIFY, point.x,
                         point.y, GetActiveWindow(), nullptr);
    if(selected && selected <= actions.size())
        actions[selected - 1]->OnClick();
}

#endif
