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

TEST_CASE_METHOD(LocaleFixture,
                 "locale reads packaged UTF-8 and XML entities") {
    assets["locale/zh_cn.xml"] =
        "\xEF\xBB\xBF<Locale><Item id='ok' text='确定 &amp; OK'/>"
        "<Item id='empty' text=''/><Item id='missing-text'/></Locale>";
    locale.Initialize("zh_cn");
    REQUIRE(locale.GetText("ok") == "确定 & OK");
    REQUIRE(locale.GetText("empty").empty());
    REQUIRE(locale.GetText("missing-text") == "missing-text");
    REQUIRE(reads == std::vector<std::string>{ "locale/zh_cn.xml" });
}

TEST_CASE_METHOD(LocaleFixture, "locale preference overrides system language") {
    GlobalConfigManager::GetInstance()->SetValue("user_language", "ja_jp");
    assets["locale/ja_jp.xml"] = "<Locale><Item id='ok' text='確認'/></Locale>";
    locale.Initialize("zh_cn");
    REQUIRE(locale.GetText("ok") == "確認");
    REQUIRE(reads == std::vector<std::string>{ "locale/ja_jp.xml" });
}

TEST_CASE_METHOD(LocaleFixture, "locale falls back once to packaged English") {
    assets["locale/en_us.xml"] = "<Locale><Item id='ok' text='OK'/></Locale>";
    locale.Initialize("unavailable");
    REQUIRE(locale.GetText("ok") == "OK");
    REQUIRE(reads ==
            std::vector<std::string>{ "locale/unavailable.xml",
                                      "locale/en_us.xml" });
}

TEST_CASE_METHOD(LocaleFixture,
                 "locale missing English returns key without recursion") {
    locale.Initialize("unavailable");
    REQUIRE(locale.GetText("ok") == "ok");
    REQUIRE(reads.size() == 2);
    reads.clear();
    locale.Initialize("en_us");
    REQUIRE(locale.GetText("ok") == "ok");
    REQUIRE(reads == std::vector<std::string>{ "locale/en_us.xml" });
}

TEST_CASE_METHOD(LocaleFixture, "locale reload clears previous translations") {
    assets["locale/en_us.xml"] =
        "<Locale><Item id='ok' text='old'/><Item id='ok' text='new'/>"
        "<Item id='previous' text='previous value'/></Locale>";
    locale.Initialize("en_us");
    REQUIRE(locale.GetText("ok") == "new");
    assets["locale/en_us.xml"] = "";
    reads.clear();
    locale.Initialize("en_us");
    REQUIRE(locale.GetText("previous") == "previous");
    REQUIRE(locale.GetText("ok") == "ok");
    REQUIRE(reads.size() == 1);
}
