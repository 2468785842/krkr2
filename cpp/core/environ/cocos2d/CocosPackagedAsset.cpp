#include "PackagedAsset.h"

#include <cocos/platform/CCFileUtils.h>

bool TVPReadPackagedAsset(
    const std::string &name,
    const std::function<void(const unsigned char *, std::size_t)> &reader) {
    const auto data = cocos2d::FileUtils::getInstance()->getDataFromFile(name);
    if(data.isNull())
        return false;
    reader(data.getBytes(), data.getSize());
    return true;
}
