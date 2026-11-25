#include "PreferenceStorage.h"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>

namespace krkr::config {
    namespace {
        std::shared_ptr<const PreferenceStorage> activeStorage;
    }

    bool PreferenceStorage::Exists(const std::string &path) const {
        std::error_code error;
        const bool exists =
            std::filesystem::exists(std::filesystem::u8path(path), error);
        // Never migrate over an inaccessible JSON file.
        return exists || bool(error);
    }

    bool PreferenceStorage::Read(const std::string &path,
                                 std::string &data) const {
        std::ifstream input(std::filesystem::u8path(path), std::ios::binary);
        if(!input)
            return false;
        std::string bytes{ std::istreambuf_iterator<char>(input), {} };
        if(input.bad())
            return false;
        data = std::move(bytes);
        return true;
    }

    bool PreferenceStorage::Write(const std::string &path,
                                  std::string_view data) const {
        if(data.size() > static_cast<std::size_t>(
                             std::numeric_limits<std::streamsize>::max()))
            return false;
        std::ofstream output(std::filesystem::u8path(path),
                             std::ios::binary | std::ios::trunc);
        if(!output)
            return false;
        output.write(data.data(), static_cast<std::streamsize>(data.size()));
        output.close();
        return !output.fail();
    }

    LocalPreferenceStorage::LocalPreferenceStorage(
        std::string writableDirectory, std::function<void()> reportFailure) :
        writable(std::move(writableDirectory)),
        reportFailure(std::move(reportFailure)) {
        if(writable.empty() || !this->reportFailure)
            throw std::invalid_argument(
                "Preference storage requires a path and error reporter");
        if(writable.back() != '/')
            writable += '/';
        preferences = writable + ".preference/";
        std::error_code error;
        std::filesystem::create_directories(
            std::filesystem::u8path(preferences), error);
        // Preserve legacy behavior: report an unwritable directory when saving.
    }

    std::string LocalPreferenceStorage::WritableDirectory() const {
        return writable;
    }
    const std::string &LocalPreferenceStorage::PreferenceDirectory() const {
        return preferences;
    }
    void LocalPreferenceStorage::ReportWriteFailure() const { reportFailure(); }

    std::string
    LinuxWritableDirectory(const std::string &executable,
                           const std::optional<std::string> &xdgConfigHome,
                           const std::optional<std::string> &home) {
        const auto name =
            std::filesystem::u8path(executable).filename().u8string();
        if(name.empty() || (!xdgConfigHome && !home))
            throw std::invalid_argument(
                "Cannot resolve Linux preference directory");
        return (xdgConfigHome ? *xdgConfigHome : *home + "/.config") + "/" +
            name + "/";
    }

    void
    SetPreferenceStorage(std::shared_ptr<const PreferenceStorage> storage) {
        std::atomic_store(&activeStorage, std::move(storage));
    }
    std::shared_ptr<const PreferenceStorage> CurrentPreferenceStorage() {
        return std::atomic_load(&activeStorage);
    }
    std::shared_ptr<const PreferenceStorage> RequirePreferenceStorage() {
        auto storage = CurrentPreferenceStorage();
        if(!storage)
            throw std::logic_error(
                "Host preference storage has not been initialized");
        return storage;
    }
    ScopedPreferenceStorage::ScopedPreferenceStorage(
        std::shared_ptr<const PreferenceStorage> storage) :
        previous(std::atomic_exchange(&activeStorage, std::move(storage))) {}
    ScopedPreferenceStorage::~ScopedPreferenceStorage() {
        std::atomic_store(&activeStorage, std::move(previous));
    }
} // namespace krkr::config

const std::string &TVPGetInternalPreferencePath() {
    return krkr::config::RequirePreferenceStorage()->PreferenceDirectory();
}
