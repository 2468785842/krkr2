#include "PackagedAsset.h"
#include "LocalAssetReader.h"

#include <stdexcept>

namespace {
    std::shared_ptr<const krkr::assets::AssetReader> RequireReader() {
        auto reader = krkr::assets::CurrentReader();
        if(!reader)
            throw std::logic_error(
                "Host asset reader has not been initialized");
        return reader;
    }
} // namespace

bool TVPHostFileExists(const std::string &name) {
    return RequireReader()->Exists(name);
}

bool TVPReadPackagedAsset(
    const std::string &name,
    const std::function<void(const unsigned char *, std::size_t)> &reader) {
    return RequireReader()->Read(name, reader);
}
