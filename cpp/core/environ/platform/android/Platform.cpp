#include "tjsCommHead.h"
#include "AndroidActivity.h"
#include "EventIntf.h"
#include "NativeDialogs.h"
#include "Platform.h"
#include "TVPLog.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <unistd.h>

#undef st_atime
#undef st_mtime
#undef st_ctime

namespace {
    std::string pathOrEmpty(const char *path) { return path ? path : ""; }

    std::vector<std::string> dialogButtons(const std::vector<ttstr> &buttons) {
        std::vector<std::string> result;
        result.reserve(buttons.size());
        for(const auto &button : buttons)
            result.push_back(button.AsStdString());
        return result;
    }
} // namespace

ttstr TVPGetPlatformName() { return TJS_W("Android"); }
ttstr TVPGetOSName() { return TVPGetPlatformName(); }
std::string TVPGetPackageVersionString() { return "android"; }

std::string TVPGetCurrentLanguage() {
    int count = 0;
    auto **locales = SDL_GetPreferredLocales(&count);
    std::unique_ptr<void, decltype(&SDL_free)> owner(locales, SDL_free);
    if(!locales || !count)
        return "en_us";
    const auto &locale = *locales[0];
    std::string language = locale.language;
    if(locale.country) {
        language += "_";
        language += locale.country;
    }
    std::transform(language.begin(), language.end(), language.begin(),
                   [](unsigned char c) {
                       return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
                   });
    return language;
}

void TVPGetMemoryInfo(TVPMemoryInfo &memory) {
    memory = {};
    std::ifstream input("/proc/meminfo");
    for(std::string key, line; input >> key;) {
        unsigned long *value = nullptr;
        if(key == "MemTotal:")
            value = &memory.MemTotal;
        else if(key == "MemAvailable:")
            value = &memory.MemFree;
        else if(key == "SwapTotal:")
            value = &memory.SwapTotal;
        else if(key == "SwapFree:")
            value = &memory.SwapFree;
        else if(key == "VmallocTotal:")
            value = &memory.VirtualTotal;
        else if(key == "VmallocUsed:")
            value = &memory.VirtualUsed;
        if(value)
            input >> *value;
        std::getline(input, line);
    }
}

tjs_int TVPGetSystemFreeMemory() {
    TVPMemoryInfo memory;
    TVPGetMemoryInfo(memory);
    if(memory.MemFree)
        return memory.MemFree / 1024;
    struct sysinfo info{};
    if(sysinfo(&info) != 0)
        return -1;
    return static_cast<uint64_t>(info.freeram) * info.mem_unit / (1024 * 1024);
}

tjs_int TVPGetSelfUsedMemory() {
    std::ifstream input("/proc/self/statm");
    uint64_t virtualPages = 0, residentPages = 0;
    if(!(input >> virtualPages >> residentPages))
        return -1;
    return residentPages * sysconf(_SC_PAGESIZE) / (1024 * 1024);
}

int TVPShowSimpleMessageBox(const ttstr &text, const ttstr &caption,
                            const std::vector<ttstr> &buttons) {
    return krkr::sdl::ShowMessage(text.AsStdString(), caption.AsStdString(),
                                  dialogButtons(buttons));
}

extern "C" int TVPShowSimpleMessageBox(const char *text, const char *caption,
                                       unsigned int count,
                                       const char **buttons) {
    std::vector<ttstr> labels;
    for(unsigned int i = 0; i < count; ++i)
        labels.emplace_back(buttons[i]);
    return TVPShowSimpleMessageBox(ttstr(text), ttstr(caption), labels);
}

int TVPShowSimpleInputBox(ttstr &text, const ttstr &caption,
                          const ttstr &prompt,
                          const std::vector<ttstr> &buttons) {
    auto value = text.AsStdString();
    const auto result =
        krkr::sdl::ShowInputBox(value, caption.AsStdString(),
                                prompt.AsStdString(), dialogButtons(buttons));
    if(result == 0)
        text = ttstr(value);
    return result;
}

bool TVP_stat(const char *name, tTVP_stat &value) {
    struct stat info{};
    if(stat(name, &info) != 0)
        return false;
    value.st_mode = info.st_mode;
    value.st_size = info.st_size;
    value.st_atime = info.st_atim.tv_sec;
    value.st_mtime = info.st_mtim.tv_sec;
    value.st_ctime = info.st_ctim.tv_sec;
    return true;
}

bool TVP_stat(const tjs_char *name, tTVP_stat &value) {
    return TVP_stat(ttstr(name).AsStdString().c_str(), value);
}

bool TVP_utime(const char *name, time_t modification) {
    const timeval times[] = { { modification, 0 }, { modification, 0 } };
    return utimes(name, times) == 0;
}

bool TVPCreateFolders(const ttstr &folder) {
    if(folder.IsEmpty())
        return true;
    const auto path = std::filesystem::u8path(folder.AsStdString());
    std::error_code error;
    return std::filesystem::create_directories(path, error) ||
        std::filesystem::is_directory(path, error);
}

bool TVPWriteDataToFile(const ttstr &file, const void *data,
                        unsigned int size) {
    std::ofstream output(std::filesystem::u8path(file.AsStdString()),
                         std::ios::binary | std::ios::trunc);
    if(!output)
        return false;
    output.write(static_cast<const char *>(data), size);
    output.close();
    return !output.fail();
}

bool TVPDeleteFile(const std::string &file) {
    return unlink(file.c_str()) == 0;
}
bool TVPRenameFile(const std::string &from, const std::string &to) {
    return rename(from.c_str(), to.c_str()) == 0;
}

std::string TVPGetDefaultFileDir() {
    return pathOrEmpty(SDL_GetAndroidInternalStoragePath());
}

ttstr Android_GetInternalStoragePath() { return ttstr(TVPGetDefaultFileDir()); }

std::vector<ttstr> Android_GetExternalStoragePath() {
    if(const auto *path = SDL_GetAndroidExternalStoragePath())
        return { ttstr(path) };
    return {};
}

std::vector<std::string> TVPGetAppStoragePath() {
    auto internal = TVPGetDefaultFileDir();
    if(const auto *external = SDL_GetAndroidExternalStoragePath())
        return { std::move(internal), external };
    return { std::move(internal) };
}

std::vector<std::string> TVPGetDriverPath() { return TVPGetAppStoragePath(); }

bool TVPCheckStartupPath(const std::string &path) {
    std::error_code error;
    return std::filesystem::exists(std::filesystem::u8path(path), error);
}

tjs_uint32 TVPGetRoughTickCount32() {
    return static_cast<tjs_uint32>(SDL_GetTicks());
}

void TVPRelinquishCPU() { sched_yield(); }
void TVPExitApplication(int code) {
    TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_MAX);
    try {
        krkr::android::ActivityCall call;
        if(call) {
            call.Environment()->CallVoidMethod(call.Activity(),
                                               call.Method("finish", "()V"));
            call.Check();
        }
    } catch(const std::exception &error) {
        G_CoreLog.warn("Could not finish Android activity: {}", error.what());
    }
    // GameActivity owns a dedicated process because engine globals are
    // initialized only once. The launcher and its persisted UI remain alive.
    std::exit(code);
}

bool TVPCheckStartupArg() { return false; }
void TVPControlAdDialog(int, int, int) {}
void TVPForceSwapBuffer() {}
void TVPProcessInputEvents() {}
void TVPSendToOtherApp(const std::string &filename) {
    if(!SDL_OpenURL(filename.c_str()))
        G_CoreLog.warn("Android could not open {}: {}", filename,
                       SDL_GetError());
}
