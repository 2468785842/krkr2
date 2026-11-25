#include <catch2/catch_test_macros.hpp>
#include "PackagedAsset.h"
#include "cocos2d/CocosPackagedAsset.h"
#include "resources/LocalAssetReader.h"

#include <chrono>
#include <iostream>

namespace {
    std::string Read(const std::string &name) {
        std::string result;
        REQUIRE(TVPHostFileExists(name));
        REQUIRE(TVPReadPackagedAsset(
            name, [&](const unsigned char *bytes, std::size_t size) {
                if(size)
                    result.assign(reinterpret_cast<const char *>(bytes), size);
            }));
        return result;
    }
} // namespace

TEST_CASE("host assets match legacy bytes for packaged font and locales") {
    REQUIRE_FALSE(krkr::assets::CurrentReader());
    krkr::assets::ScopedReader legacy(TVPCreateCocosAssetReader());
    const auto root = std::filesystem::u8path(KRKR_ASSET_DIR);
    const auto reader = std::make_shared<krkr::assets::LocalAssetReader>(
        std::vector<std::filesystem::path>{ root });
    for(const std::string name :
        { "locale/en_us.json", "locale/zh_cn.json", "NotoSansCJK-Regular.ttc" }) {
        CAPTURE(name);
        const auto before = std::chrono::steady_clock::now();
        const auto expected = Read((root / name).u8string());
        const auto baseline = std::chrono::steady_clock::now();
        std::string actual;
        {
            krkr::assets::ScopedReader native(reader);
            actual = Read(name);
        }
        const auto after = std::chrono::steady_clock::now();
        REQUIRE_FALSE(expected.empty());
        REQUIRE(actual == expected);
        // A same-process I/O baseline, not a performance threshold.
        std::cout << name << " bytes=" << actual.size() << " cocos_ms="
                  << std::chrono::duration<double, std::milli>(baseline -
                                                               before)
                         .count()
                  << " native_ms="
                  << std::chrono::duration<double, std::milli>(after - baseline)
                         .count()
                  << '\n';
    }
    REQUIRE(krkr::assets::CurrentReader());
}
