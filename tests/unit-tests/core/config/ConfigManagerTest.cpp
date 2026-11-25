#include <catch2/catch_test_macros.hpp>

#include "ConfigManager/ConfigFormat.h"
#include "ConfigManager/GlobalConfigManager.h"
#include "ConfigManager/IndividualConfigManager.h"
#include "ConfigManager/PreferenceStorage.h"
#include "ConfigManager/LocalHostServices.h"
#include "PackagedAsset.h"
#include "resources/LocalAssetReader.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>

namespace {
    struct MemoryStorage final : krkr::config::PreferenceStorage {
        std::string directory = "test/";
        mutable std::string xml;
        bool Exists(const std::string &) const override { return !xml.empty(); }
        mutable std::string lastPath;
        mutable int writes = 0;
        mutable int failures = 0;
        bool writable = true;
        std::string WritableDirectory() const override { return directory; }
        const std::string &PreferenceDirectory() const override {
            return directory;
        }
        bool Read(const std::string &, std::string &data) const override {
            data = xml;
            return !xml.empty();
        }
        bool Write(const std::string &path,
                   std::string_view data) const override {
            ++writes;
            lastPath = path;
            if(!writable)
                return false;
            xml = data;
            return true;
        }
        void ReportWriteFailure() const override { ++failures; }
    };
    struct Manager : iSysConfigManager {
        std::string GetFilePath() override { return "test/preferences.json"; }
        Manager() { Initialize(); }
    };
    struct ConfigFixture {
        std::shared_ptr<MemoryStorage> memory =
            std::make_shared<MemoryStorage>();
        krkr::config::ScopedPreferenceStorage storage{ memory };
        ConfigFixture() { iSysConfigManager::SetLaunchOverrides({}); }
        ~ConfigFixture() { iSysConfigManager::SetLaunchOverrides({}); }
    };
} // namespace

TEST_CASE("native host services cover configuration before window startup") {
    const auto previousReader = krkr::assets::CurrentReader();
    const auto previousStorage = krkr::config::CurrentPreferenceStorage();
    const auto root = std::filesystem::temp_directory_path() /
        ("krkr-services-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    } cleanup{ root };
    std::filesystem::create_directories(root / "Resources");
    {
        std::ofstream file(root / "Resources/font.bin", std::ios::binary);
        file.write("A\0B", 3);
    }
    int failures = 0;
    {
        krkr::host::LocalServices services(root / "Resources",
                                           (root / "settings").u8string(),
                                           [&] { ++failures; });
        REQUIRE(TVPHostFileExists("font.bin"));
        std::string bytes;
        REQUIRE(TVPReadPackagedAsset(
            "font.bin", [&](const unsigned char *data, std::size_t size) {
                bytes.assign(reinterpret_cast<const char *>(data), size);
            }));
        REQUIRE(bytes == std::string("A\0B", 3));
        const auto storage = krkr::config::RequirePreferenceStorage();
        const auto path = storage->PreferenceDirectory() + "global.json";
        struct LocalManager : iSysConfigManager {
            std::string path;
            explicit LocalManager(std::string path) : path(std::move(path)) {
                Initialize();
            }
            std::string GetFilePath() override { return path; }
        } manager(path);
        manager.SetValueInt("volume", 17);
        manager.SaveToFile();
        LocalManager reloaded(path);
        REQUIRE(reloaded.GetValue<int>("volume", 0) == 17);
        REQUIRE_FALSE(storage->Write((root / "missing/file").u8string(), "x"));
        storage->ReportWriteFailure();
        REQUIRE(failures == 1);
        {
            krkr::host::LocalServices nested(
                root / "empty", (root / "nested").u8string(), [] {});
            REQUIRE_FALSE(TVPHostFileExists("font.bin"));
        }
        REQUIRE(krkr::config::CurrentPreferenceStorage() == storage);
        REQUIRE(TVPHostFileExists("font.bin"));
    }
    REQUIRE(krkr::assets::CurrentReader() == previousReader);
    REQUIRE(krkr::config::CurrentPreferenceStorage() == previousStorage);
}

TEST_CASE("failed native service construction restores previous providers") {
    const auto previousReader = krkr::assets::CurrentReader();
    const auto previousStorage = krkr::config::CurrentPreferenceStorage();
    REQUIRE_THROWS_AS((krkr::host::LocalServices(
                          std::filesystem::temp_directory_path(), "", [] {})),
                      std::invalid_argument);
    REQUIRE(krkr::assets::CurrentReader() == previousReader);
    REQUIRE(krkr::config::CurrentPreferenceStorage() == previousStorage);
}

TEST_CASE("config Linux preference path preserves legacy environment rules") {
    using krkr::config::LinuxWritableDirectory;
    REQUIRE(LinuxWritableDirectory("/opt/games/krkr2", "/custom",
                                   "/home/user") == "/custom/krkr2/");
    REQUIRE(
        LinuxWritableDirectory("/opt/renamed", std::nullopt, "/home/user") ==
        "/home/user/.config/renamed/");
    REQUIRE(LinuxWritableDirectory("/opt/krkr2", "", std::nullopt) ==
            "/krkr2/");
    REQUIRE_THROWS_AS(
        LinuxWritableDirectory("/opt/krkr2", std::nullopt, std::nullopt),
        std::invalid_argument);
}

TEST_CASE("config preference providers restore scope and reject missing host") {
    const auto previous = krkr::config::CurrentPreferenceStorage();
    {
        krkr::config::ScopedPreferenceStorage missing(nullptr);
        REQUIRE_THROWS_AS(krkr::config::RequirePreferenceStorage(),
                          std::logic_error);
        {
            auto memory = std::make_shared<MemoryStorage>();
            krkr::config::ScopedPreferenceStorage scope(memory);
            REQUIRE(krkr::config::CurrentPreferenceStorage() == memory);
            REQUIRE(TVPGetInternalPreferencePath() == "test/");
        }
        REQUIRE_FALSE(krkr::config::CurrentPreferenceStorage());
    }
    REQUIRE(krkr::config::CurrentPreferenceStorage() == previous);
}

TEST_CASE("config local storage creates UTF-8 paths and reads exact bytes") {
    const auto root = std::filesystem::temp_directory_path() /
        ("krkr-config-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto writable = root / std::filesystem::u8path("设置");
    int failures = 0;
    krkr::config::LocalPreferenceStorage storage(writable.u8string(),
                                                 [&] { ++failures; });
    const auto file = std::filesystem::u8path(storage.PreferenceDirectory()) /
        "GlobalPreference.json";
    const std::string bytes("a\0b\r\n", 5);
    REQUIRE(storage.Write(file.u8string(), bytes));
    std::string actual;
    REQUIRE(storage.Read(file.u8string(), actual));
    REQUIRE(actual == bytes);
    REQUIRE(storage.Write(file.u8string(), ""));
    REQUIRE(storage.Read(file.u8string(), actual));
    REQUIRE(actual.empty());
    REQUIRE_FALSE(storage.Read((root / "missing").u8string(), actual));
    REQUIRE_FALSE(storage.Write(writable.u8string(), bytes));
    storage.ReportWriteFailure();
    REQUIRE(failures == 1);
    // Only remove the files/directories created by this test; no recursive
    // delete.
    std::filesystem::remove(file);
    std::filesystem::remove(file.parent_path());
    std::filesystem::remove(writable);
    std::filesystem::remove(root);
}

TEST_CASE_METHOD(ConfigFixture,
                 "config manager roundtrip excludes launch overrides") {
    memory->xml =
        krkr::config::WritePreferences({ { { "volume", "20" } },
                                         { { "a", "1" }, { "a", "2" } },
                                         { { 65, 66 } } });
    Manager manager;
    REQUIRE(manager.GetValue<int>("volume", 0) == 20);
    REQUIRE(manager.GetCustomArgumentsForPush() ==
            std::vector<std::string>{ "-a=1", "-a=2" });
    REQUIRE(manager.GetKeyMap().at(65) == 66);
    iSysConfigManager::SetLaunchOverrides({ { "volume", "99" } });
    REQUIRE(manager.GetValue<int>("volume", 0) == 99);
    manager.SetValue("language", "中文 & English");
    manager.SaveToFile();
    REQUIRE(memory->writes == 1);
    const auto preferences = krkr::config::ReadPreferences(memory->xml);
    REQUIRE(preferences);
    REQUIRE(preferences->values.at("volume") == "20");
    REQUIRE(preferences->values.at("language") == "中文 & English");
    iSysConfigManager::SetLaunchOverrides({});
    Manager reloaded;
    REQUIRE(reloaded.GetValue<int>("volume", 0) == 20);
    REQUIRE(reloaded.GetCustomArguments() == manager.GetCustomArguments());
    REQUIRE(reloaded.GetKeyMap() == manager.GetKeyMap());
    reloaded.SaveToFile();
    REQUIRE(memory->writes == 1);

    auto *global = GlobalConfigManager::GetInstance();
    global->SetValueInt("volume", 7);
    auto *individual = IndividualConfigManager::GetInstance();
    REQUIRE(individual->CreatePreferenceAt("game"));
    REQUIRE(individual->GetValue<int>("volume", 0) == 7);
    individual->SetValueInt("volume", 8);
    REQUIRE(individual->GetValue<int>("volume", 0) == 8);
    REQUIRE(global->GetValue<int>("volume", 0) == 7);
    individual->SaveToFile();
    REQUIRE(memory->lastPath == "game/Kirikiroid2Preference.json");
    REQUIRE(krkr::config::ReadPreferences(memory->xml)->values.at("volume") ==
            "8");
}

TEST_CASE_METHOD(
    ConfigFixture,
    "config manager reports failed writes with legacy dirty state") {
    memory->writable = false;
    Manager manager;
    manager.SetValueInt("volume", 12);
    manager.SaveToFile();
    REQUIRE(memory->writes == 1);
    REQUIRE(memory->failures == 1);
    // Existing behavior clears the dirty flag even after a failed save.
    manager.SaveToFile();
    REQUIRE(memory->writes == 1);
    manager.SetValueInt("volume", 13);
    manager.SaveToFile();
    REQUIRE(memory->failures == 2);
}

TEST_CASE("config migrates legacy XML once and gives JSON precedence") {
    const auto root = std::filesystem::temp_directory_path() /
        ("krkr-json-migration-" +
         std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(root);
    struct Cleanup {
        std::filesystem::path root;
        ~Cleanup() { std::filesystem::remove_all(root); }
    } cleanup{ root };
    auto local = std::make_shared<krkr::config::LocalPreferenceStorage>(
        root.u8string(), [] {});
    krkr::config::ScopedPreferenceStorage storage(local);
    const auto json = (root / "preferences.json").u8string();
    const auto xml = (root / "preferences.xml").u8string();
    const std::string old =
        "<GlobalPreference><Item key='name' value='中文 &amp; old'/><Custom "
        "key='a' value='1'/><Custom key='a' value='2'/><KeyMap key='65' "
        "value='66'/></GlobalPreference>";
    REQUIRE(local->Write(xml, old));
    struct LocalManager : iSysConfigManager {
        std::string path;
        explicit LocalManager(std::string path) : path(std::move(path)) {
            Initialize();
        }
        std::string GetFilePath() override { return path; }
    } migrated(json);
    REQUIRE(migrated.GetValue<std::string>("name", "") == "中文 & old");
    REQUIRE(migrated.GetCustomArguments().size() == 2);
    REQUIRE(migrated.GetKeyMap().at(65) == 66);
    std::string actual;
    REQUIRE(local->Read(json, actual));
    REQUIRE(krkr::config::ReadPreferences(actual));
    REQUIRE(local->Read(xml, actual));
    REQUIRE(actual == old);
    REQUIRE(local->Write(xml,
                         "<GlobalPreference><Item key='name' "
                         "value='stale'/></GlobalPreference>"));
    LocalManager reloaded(json);
    REQUIRE(reloaded.GetValue<std::string>("name", "") == "中文 & old");
    REQUIRE(local->Write(json, "broken JSON"));
    LocalManager malformed(json);
    REQUIRE_FALSE(malformed.IsValueExist("name"));
    REQUIRE(local->Read(json, actual));
    REQUIRE(actual == "broken JSON");
    REQUIRE(local->Write(json, ""));
    LocalManager empty(json);
    REQUIRE_FALSE(empty.IsValueExist("name"));

    krkr::assets::ScopedReader assets(
        std::make_shared<krkr::assets::LocalAssetReader>(
            std::vector<std::filesystem::path>{ root }));
    const auto game = (root / "game").u8string();
    std::filesystem::create_directory(std::filesystem::u8path(game));
    REQUIRE(local->Write(game + "/Kirikiroid2Preference.xml", old));
    REQUIRE(IndividualConfigManager::CheckExistAt(game));
    auto *individual = IndividualConfigManager::GetInstance();
    REQUIRE(individual->UsePreferenceAt(game));
    REQUIRE(individual->GetValue<std::string>("name", "") == "中文 & old");
    REQUIRE(local->Exists(game + "/Kirikiroid2Preference.json"));
    REQUIRE(individual->GetKeyMap().at(65) == 66);
    REQUIRE_FALSE(
        individual->UsePreferenceAt((root / "missing-game").u8string()));
    REQUIRE(individual->GetKeyMap().empty());
    REQUIRE(individual->GetCustomArguments().empty());
    std::filesystem::remove(
        std::filesystem::u8path(game + "/Kirikiroid2Preference.xml"));
    REQUIRE(IndividualConfigManager::CheckExistAt(game));
    REQUIRE(individual->UsePreferenceAt(game));
    REQUIRE(individual->GetKeyMap().at(65) == 66);
    REQUIRE(individual->CreatePreferenceAt((root / "new-game").u8string()));
    REQUIRE(individual->GetKeyMap().empty());

    REQUIRE(local->Write((root / "invalid.xml").u8string(), "<broken>"));
    LocalManager invalid((root / "invalid.json").u8string());
    REQUIRE_FALSE(local->Exists((root / "invalid.json").u8string()));

    struct FailingStorage : krkr::config::PreferenceStorage {
        std::string legacy;
        mutable int failures = 0;
        std::string WritableDirectory() const override { return {}; }
        const std::string &PreferenceDirectory() const override {
            return legacy;
        }
        bool Exists(const std::string &) const override { return false; }
        bool Read(const std::string &, std::string &data) const override {
            data = legacy;
            return true;
        }
        bool Write(const std::string &, std::string_view) const override {
            return false;
        }
        void ReportWriteFailure() const override { ++failures; }
    };
    auto failing = std::make_shared<FailingStorage>();
    failing->legacy = old;
    krkr::config::ScopedPreferenceStorage failedStorage(failing);
    LocalManager failed("unwritable.json");
    REQUIRE(failing->failures == 1);
    REQUIRE(failing->legacy == old);
    REQUIRE(failed.GetValue<std::string>("name", "") == "中文 & old");
}
