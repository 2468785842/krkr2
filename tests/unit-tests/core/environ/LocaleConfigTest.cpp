#include <catch2/catch_test_macros.hpp>

#include "ConfigManager/GlobalConfigManager.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "resources/LocalAssetReader.h"
#include "ConfigManager/PreferenceStorage.h"

#include <map>

namespace {
    // Test host: preferences and packaged bytes, without a graphics context.
    std::map<std::string, std::string> assets;
    std::vector<std::string> reads;

    class MemoryStorage final : public krkr::config::PreferenceStorage {
        std::string directory;

    public:
        std::string WritableDirectory() const override { return {}; }
        const std::string &PreferenceDirectory() const override {
            return directory;
        }
        bool Read(const std::string &, std::string &) const override {
            return false;
        }
        bool Write(const std::string &, std::string_view) const override {
            return true;
        }
        void ReportWriteFailure() const override {}
    };
    class MemoryAssets final : public krkr::assets::AssetReader {
    public:
        bool Exists(const std::string &name) const override {
            return assets.count(name);
        }
        bool Read(const std::string &name,
                  const krkr::assets::Reader &reader) const override {
            reads.push_back(name);
            const auto found = assets.find(name);
            if(found == assets.end())
                return false;
            const auto &data = found->second;
            reader(reinterpret_cast<const unsigned char *>(data.data()),
                   data.size());
            return true;
        }
    };
    struct LocaleFixture {
        krkr::config::ScopedPreferenceStorage storage{
            std::make_shared<MemoryStorage>()
        };
        krkr::assets::ScopedReader reader{ std::make_shared<MemoryAssets>() };
        LocaleFixture() {
            iSysConfigManager::SetLaunchOverrides({});
            GlobalConfigManager::GetInstance()->SetValue("user_language", "");
            assets.clear();
            reads.clear();
        }
        LocaleConfigManager &locale = *LocaleConfigManager::GetInstance();
    };
} // namespace

TEST_CASE_METHOD(LocaleFixture, "locale reads packaged UTF-8 JSON") {
    assets["locale/zh_cn.json"] = "\xEF\xBB\xBF"
                                  R"({"ok":"确定 & OK","empty":""})";
    locale.Initialize("zh_cn");
    REQUIRE(locale.GetText("ok") == "确定 & OK");
    REQUIRE(locale.GetText("empty").empty());
    REQUIRE(locale.GetText("missing-text") == "missing-text");
    REQUIRE(reads == std::vector<std::string>{ "locale/zh_cn.json" });
}

TEST_CASE_METHOD(LocaleFixture, "locale preference overrides system language") {
    GlobalConfigManager::GetInstance()->SetValue("user_language", "ja_jp");
    assets["locale/ja_jp.json"] = R"({"ok":"確認"})";
    locale.Initialize("zh_cn");
    REQUIRE(locale.GetText("ok") == "確認");
    REQUIRE(reads == std::vector<std::string>{ "locale/ja_jp.json" });
}

TEST_CASE_METHOD(LocaleFixture, "locale falls back once to packaged English") {
    assets["locale/en_us.json"] = R"({"ok":"OK"})";
    locale.Initialize("unavailable");
    REQUIRE(locale.GetText("ok") == "OK");
    REQUIRE(reads ==
            std::vector<std::string>{ "locale/unavailable.json",
                                      "locale/en_us.json" });
}

TEST_CASE_METHOD(LocaleFixture,
                 "locale missing English returns key without recursion") {
    locale.Initialize("unavailable");
    REQUIRE(locale.GetText("ok") == "ok");
    REQUIRE(reads.size() == 2);
    reads.clear();
    locale.Initialize("en_us");
    REQUIRE(locale.GetText("ok") == "ok");
    REQUIRE(reads == std::vector<std::string>{ "locale/en_us.json" });
}

TEST_CASE_METHOD(LocaleFixture, "locale reload clears previous translations") {
    assets["locale/en_us.json"] =
        R"({"ok":"old","ok":"new","previous":"previous value"})";
    locale.Initialize("en_us");
    REQUIRE(locale.GetText("ok") == "new");
    assets["locale/en_us.json"] = "";
    reads.clear();
    locale.Initialize("en_us");
    REQUIRE(locale.GetText("previous") == "previous");
    REQUIRE(locale.GetText("ok") == "ok");
    REQUIRE(reads.size() == 1);
}
