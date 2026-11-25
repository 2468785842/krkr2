#include "LocaleConfigManager.h"
#include "PackagedAsset.h"
#include "GlobalConfigManager.h"
#include "ConfigFormat.h"

LocaleConfigManager::LocaleConfigManager() = default;

LocaleConfigManager *LocaleConfigManager::GetInstance() {
    static LocaleConfigManager instance;
    return &instance;
}

const std::string &LocaleConfigManager::GetText(const std::string &tid) {
    auto it = AllConfig.find(tid);
    if(it == AllConfig.end()) {
        AllConfig[tid] = tid;
        return AllConfig[tid];
    }
    return it->second;
}

void LocaleConfigManager::Initialize(const std::string &sysLang) {
    // override by global configured lang
    currentLangCode = GlobalConfigManager::GetInstance()->GetValue<std::string>(
        "user_language", "");
    if(currentLangCode.empty())
        currentLangCode = sysLang;
    AllConfig.clear();
    std::string jsonData;
    const auto readLocale = [&](const std::string &language) {
        return TVPReadPackagedAsset(
            "locale/" + language + ".json",
            [&](const unsigned char *bytes, std::size_t size) {
                if(size)
                    jsonData.assign(reinterpret_cast<const char *>(bytes),
                                    size);
            });
    };
    if(!readLocale(currentLangCode) && currentLangCode != "en_us") {
        currentLangCode = "en_us";
        readLocale(currentLangCode);
    }
    AllConfig = krkr::config::ReadLocale(jsonData);
}
