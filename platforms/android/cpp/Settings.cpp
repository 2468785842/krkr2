#include "JniStrings.h"
#include "ConfigManager/ConfigFormat.h"
#include "ConfigManager/PreferenceDefaults.h"
#include "ConfigManager/PreferenceStorage.h"
#include "launch/DesktopSettings.h"
#include "launch/LaunchOptions.h"

#include <filesystem>
#include <sstream>
#include <unistd.h>

namespace {
    using namespace krkr::android;

    krkr::config::Preferences
    readPreferences(const krkr::config::PreferenceStorage &storage,
                    const std::string &path, const std::string &legacyPath) {
        const bool json = storage.Exists(path);
        const auto &file = json ? path : legacyPath;
        if(!json && !storage.Exists(file))
            return {};
        std::string bytes;
        if(!storage.Read(file, bytes))
            throw std::runtime_error("Cannot read " + file);
        auto preferences = json ? krkr::config::ReadPreferences(bytes)
                                : krkr::config::ReadLegacyPreferences(bytes);
        if(!preferences)
            throw std::runtime_error("Invalid preferences in " + file);
        return std::move(*preferences);
    }

    krkr::config::Values defaults() {
        return {
#define TVP_BOOL_DEFAULT(name, value) { #name, value ? "1" : "0" },
#define TVP_FLOAT_DEFAULT(name, value)                                         \
    { #name, std::to_string(TVPPreferenceDefaults::value_##name) },
#define TVP_STRING_DEFAULT(name, value) { #name, value },
#include "ConfigManager/PreferenceDefaults.inc"
#undef TVP_BOOL_DEFAULT
#undef TVP_FLOAT_DEFAULT
#undef TVP_STRING_DEFAULT
            { "virtual_mouse", "0" },
        };
    }

    void report(JNIEnv *env, const std::exception &error) {
        if(env->ExceptionCheck())
            return;
        auto type = env->FindClass("java/lang/IllegalArgumentException");
        if(!type)
            return;
        auto constructor =
            env->GetMethodID(type, "<init>", "(Ljava/lang/String;)V");
        auto message = ToJava(env, std::string_view(error.what()));
        auto exception =
            static_cast<jthrowable>(env->NewObject(type, constructor, message));
        if(exception)
            env->Throw(exception);
    }
} // namespace

extern "C" JNIEXPORT jobjectArray JNICALL
Java_org_github_krkr2_EngineSettings_read(JNIEnv *env, jclass, jstring root) {
    try {
        const krkr::config::LocalPreferenceStorage storage(FromJava(env, root),
                                                           [] {});
        const auto &folder = storage.PreferenceDirectory();
        auto saved = readPreferences(storage, folder + "GlobalPreference.json",
                                     folder + "GlobalPreference.xml");
        const auto fallback = defaults();
        std::istringstream list(krkr::launch::ListDesktopSettings());
        std::vector<std::string> rows;
        for(std::string line; std::getline(list, line);) {
            const auto separator = line.find('\t');
            const auto key = line.substr(0, separator);
            const auto found = saved.values.find(key);
            const auto value = found == saved.values.end()
                ? (fallback.count(key) ? fallback.at(key) : std::string{})
                : found->second;
            // Display the effective default if an older preference contains
            // a value outside the current CLI contract.
            std::string validated = value;
            try {
                std::map<std::string, std::string> settings;
                krkr::launch::AddDesktopSetting(settings, key + "=" + value);
            } catch(const std::invalid_argument &) {
                validated = fallback.count(key) ? fallback.at(key) : "";
            }
            rows.insert(rows.end(),
                        { key, line.substr(separator + 1), validated });
        }
        return ToJava(env, rows);
    } catch(const std::exception &error) {
        report(env, error);
        return nullptr;
    }
}

extern "C" JNIEXPORT void JNICALL Java_org_github_krkr2_EngineSettings_save(
    JNIEnv *env, jclass, jstring root, jobjectArray assignments) {
    try {
        std::map<std::string, std::string> settings;
        for(const auto &assignment : FromJava(env, assignments))
            krkr::launch::AddDesktopSetting(settings, assignment);

        const krkr::config::LocalPreferenceStorage storage(FromJava(env, root),
                                                           [] {});
        const auto &folder = storage.PreferenceDirectory();
        const auto path = folder + "GlobalPreference.json";
        auto saved =
            readPreferences(storage, path, folder + "GlobalPreference.xml");
        for(const auto &[key, value] : settings)
            saved.values[key] = value;
        // Copy and publish in the same directory so the game process sees a
        // complete JSON file even if it reads while the launcher saves.
        const auto temporary = path + "." + std::to_string(getpid()) + ".tmp";
        const auto json = krkr::config::WritePreferences(saved);
        std::error_code error;
        if(storage.Write(temporary, json))
            std::filesystem::rename(temporary, path, error);
        else
            error = std::make_error_code(std::errc::io_error);
        if(error) {
            std::error_code ignored;
            std::filesystem::remove(temporary, ignored);
            throw std::runtime_error("Cannot save " + path + ": " +
                                     error.message());
        }
    } catch(const std::exception &error) {
        report(env, error);
    }
}

extern "C" JNIEXPORT void JNICALL
Java_org_github_krkr2_EngineSettings_checkLaunch(JNIEnv *env, jclass,
                                                 jobjectArray arguments) {
    try {
        const auto options = krkr::launch::Parse(FromJava(env, arguments));
        if(options.game)
            krkr::launch::ResolveTarget(*options.game);
    } catch(const std::exception &error) {
        report(env, error);
    }
}
