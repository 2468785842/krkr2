#include "CocosPreferenceStorage.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "Platform.h"
#include "StorageImpl.h"
#include "platform/CCFileUtils.h"

#include <limits>

bool TVPWriteDataToFile(const ttstr &filepath, const void *data,
                        unsigned int len);

namespace {
    class CocosPreferenceStorage final
        : public krkr::config::PreferenceStorage {
    public:
        std::string WritableDirectory() const override {
            return cocos2d::FileUtils::getInstance()->getWritablePath();
        }
        const std::string &PreferenceDirectory() const override {
            static const auto directory = [this] {
                auto path = WritableDirectory() + ".preference";
                if(!TVPCheckExistentLocalFolder(path))
                    TVPCreateFolders(path);
                return path + "/";
            }();
            return directory;
        }
        bool Write(const std::string &path,
                   std::string_view data) const override {
            return data.size() <= std::numeric_limits<unsigned int>::max() &&
                TVPWriteDataToFile(path, data.data(),
                                   static_cast<unsigned int>(data.size()));
        }
        void ReportWriteFailure() const override {
            auto *locale = LocaleConfigManager::GetInstance();
            TVPShowSimpleMessageBox(locale->GetText("cannot_create_preference"),
                                    locale->GetText("readonly_storage"));
        }
    };
} // namespace

std::shared_ptr<const krkr::config::PreferenceStorage>
TVPCreateCocosPreferenceStorage() {
    return std::make_shared<CocosPreferenceStorage>();
}
