#include <catch2/catch_test_macros.hpp>
#include "resources/LocalAssetReader.h"
#include "PackagedAsset.h"

#include <chrono>
#include <fstream>
#include <stdexcept>

namespace {
    struct AssetFixture {
        std::filesystem::path root = std::filesystem::temp_directory_path() /
            ("krkr-assets-" +
             std::to_string(
                 std::chrono::steady_clock::now().time_since_epoch().count()));
        AssetFixture() {
            std::filesystem::create_directories(root / "first");
            std::filesystem::create_directories(root / "second");
        }
        ~AssetFixture() {
            std::error_code error;
            const auto target =
                std::filesystem::absolute(root).lexically_normal();
            const auto temp = std::filesystem::absolute(
                                  std::filesystem::temp_directory_path())
                                  .lexically_normal();
            if(target.parent_path() == temp &&
               target.filename().u8string().find("krkr-assets-") == 0)
                std::filesystem::remove_all(target, error);
        }
        void Write(const std::filesystem::path &name,
                   const std::string &bytes) {
            std::ofstream output(root / name, std::ios::binary);
            output.write(bytes.data(),
                         static_cast<std::streamsize>(bytes.size()));
            REQUIRE(output.good());
        }
        std::string Read(const krkr::assets::AssetReader &assets,
                         const std::string &name) {
            std::string bytes;
            REQUIRE(assets.Read(
                name, [&](const unsigned char *data, std::size_t size) {
                    if(size)
                        bytes.assign(reinterpret_cast<const char *>(data),
                                     size);
                }));
            return bytes;
        }
    };
} // namespace

TEST_CASE_METHOD(AssetFixture,
                 "host assets resolve ordered roots and absolute local files") {
    Write("first/shared", "first");
    Write("second/shared", "second");
    Write("second/fallback", "fallback");
    krkr::assets::LocalAssetReader assets({ root / "first", root / "second" });
    REQUIRE(Read(assets, "shared") == "first");
    REQUIRE(Read(assets, "fallback") == "fallback");
    REQUIRE(Read(assets, (root / "second/shared").u8string()) == "second");
    REQUIRE_FALSE(assets.Exists("missing"));
    REQUIRE_FALSE(assets.Exists(""));
    REQUIRE_FALSE(assets.Exists((root / "first").u8string()));
}

TEST_CASE_METHOD(
    AssetFixture,
    "host assets preserve UTF-8 file names empty and binary bytes") {
    const auto name = std::filesystem::u8path("中文.bin");
    const std::string binary("a\0b\r\n\xff", 6);
    Write(std::filesystem::path("first") / name, binary);
    Write("first/empty", "");
    krkr::assets::LocalAssetReader assets({ root / "first" });
    REQUIRE(assets.Exists(name.u8string()));
    REQUIRE(Read(assets, name.u8string()) == binary);
    REQUIRE(assets.Exists("empty"));
    REQUIRE(Read(assets, "empty").empty());
}

TEST_CASE_METHOD(
    AssetFixture,
    "host assets do not use the current working directory as fallback") {
    // The absolute name is readable; a relative query requires an explicit
    // root.
    Write("outside", "outside");
    krkr::assets::LocalAssetReader assets({ root / "first" });
    REQUIRE_FALSE(assets.Exists("outside"));
    REQUIRE(Read(assets, (root / "outside").u8string()) == "outside");
    bool called = false;
    REQUIRE_FALSE(assets.Read(
        "missing", [&](const unsigned char *, std::size_t) { called = true; }));
    REQUIRE_FALSE(called);
}

TEST_CASE_METHOD(
    AssetFixture,
    "host asset reader scopes restore and readers retain ownership") {
    REQUIRE_FALSE(krkr::assets::CurrentReader());
    REQUIRE_THROWS_AS(TVPHostFileExists("file"), std::logic_error);
    Write("first/file", "first");
    Write("second/file", "second");
    std::shared_ptr<const krkr::assets::AssetReader> retained;
    {
        krkr::assets::ScopedReader first(
            std::make_shared<krkr::assets::LocalAssetReader>(
                std::vector<std::filesystem::path>{ root / "first" }));
        retained = krkr::assets::CurrentReader();
        REQUIRE(TVPHostFileExists("file"));
        REQUIRE(TVPReadPackagedAsset(
            "file", [](const unsigned char *bytes, std::size_t size) {
                REQUIRE(std::string(reinterpret_cast<const char *>(bytes),
                                    size) == "first");
            }));
        {
            krkr::assets::ScopedReader second(
                std::make_shared<krkr::assets::LocalAssetReader>(
                    std::vector<std::filesystem::path>{ root / "second" }));
            REQUIRE(Read(*krkr::assets::CurrentReader(), "file") == "second");
        }
        REQUIRE(Read(*krkr::assets::CurrentReader(), "file") == "first");
    }
    REQUIRE_FALSE(krkr::assets::CurrentReader());
    REQUIRE(Read(*retained, "file") == "first");
}

TEST_CASE_METHOD(
    AssetFixture,
    "host asset callbacks propagate exceptions without retaining handles") {
    Write("first/file", "data");
    krkr::assets::LocalAssetReader assets({ root / "first" });
    bool removed = false;
    REQUIRE_THROWS_AS(assets.Read("file",
                                  [&](const unsigned char *, std::size_t) {
                                      removed = std::filesystem::remove(
                                          root / "first/file");
                                      throw std::runtime_error("reader");
                                  }),
                      std::runtime_error);
    REQUIRE(removed);
    REQUIRE_FALSE(assets.Exists("file"));
}
