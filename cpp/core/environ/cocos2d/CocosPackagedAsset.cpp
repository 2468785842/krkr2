#include "CocosPackagedAsset.h"

#include <cocos/platform/CCFileUtils.h>

namespace {
    class CocosAssetReader final : public krkr::assets::AssetReader {
    public:
        bool Exists(const std::string &name) const override {
            return cocos2d::FileUtils::getInstance()->isFileExist(name);
        }
        bool Read(const std::string &name,
                  const krkr::assets::Reader &reader) const override {
            const auto data =
                cocos2d::FileUtils::getInstance()->getDataFromFile(name);
            if(data.isNull())
                return false;
            reader(data.getBytes(), data.getSize());
            return true;
        }
    };
} // namespace

std::shared_ptr<const krkr::assets::AssetReader> TVPCreateCocosAssetReader() {
    return std::make_shared<CocosAssetReader>();
}
