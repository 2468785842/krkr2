#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace krkr::config {
    class PreferenceStorage {
    public:
        virtual ~PreferenceStorage() = default;
        virtual std::string WritableDirectory() const = 0;
        virtual const std::string &PreferenceDirectory() const = 0;
        virtual bool Read(const std::string &path, std::string &data) const;
        virtual bool Write(const std::string &path,
                           std::string_view data) const;
        virtual void ReportWriteFailure() const = 0;
    };

    class LocalPreferenceStorage final : public PreferenceStorage {
        std::string writable;
        std::string preferences;
        std::function<void()> reportFailure;

    public:
        LocalPreferenceStorage(std::string writableDirectory,
                               std::function<void()> reportFailure);
        std::string WritableDirectory() const override;
        const std::string &PreferenceDirectory() const override;
        void ReportWriteFailure() const override;
    };

    // Matches the legacy Linux FileUtils path, including an explicitly empty
    // XDG_CONFIG_HOME. Missing HOME is an error when XDG_CONFIG_HOME is unset.
    std::string
    LinuxWritableDirectory(const std::string &executable,
                           const std::optional<std::string> &xdgConfigHome,
                           const std::optional<std::string> &home);

    void SetPreferenceStorage(std::shared_ptr<const PreferenceStorage> storage);
    std::shared_ptr<const PreferenceStorage> CurrentPreferenceStorage();
    std::shared_ptr<const PreferenceStorage> RequirePreferenceStorage();

    // Install/restore on the host thread, covering engine initialization and
    // shutdown. Workers retain a complete provider through shared_ptr.
    class ScopedPreferenceStorage {
        std::shared_ptr<const PreferenceStorage> previous;

    public:
        explicit ScopedPreferenceStorage(
            std::shared_ptr<const PreferenceStorage> storage);
        ~ScopedPreferenceStorage();
        ScopedPreferenceStorage(const ScopedPreferenceStorage &) = delete;
        ScopedPreferenceStorage &
        operator=(const ScopedPreferenceStorage &) = delete;
    };
} // namespace krkr::config

// The host must keep its provider installed while callers use this reference.
const std::string &TVPGetInternalPreferencePath();
