#include "RenderSettings.h"
#include <catch2/catch_test_macros.hpp>
#include <limits>
#include <map>
#include <vector>

namespace {
    struct Source : krkr::render::PreferenceSource {
        std::map<std::string, std::string> strings;
        std::vector<std::string> reads;
        std::string String(const char *key, const char *fallback) override {
            reads.emplace_back(key);
            return strings.emplace(key, fallback).first->second;
        }
        int Integer(const char *, int fallback) override { return fallback; }
    };
} // namespace

TEST_CASE(
    "render preferences request defaults only at the original read points") {
    Source source;
    REQUIRE(krkr::render::RendererName(source) == "software");
    REQUIRE(source.reads == std::vector<std::string>{ "renderer" });
    REQUIRE(source.strings.size() == 1);
    REQUIRE(krkr::render::SoftwareStorage(source) ==
            krkr::render::TextureStorage::Normal);
    REQUIRE(source.reads ==
            std::vector<std::string>{ "renderer", "software_compress_tex" });
    REQUIRE(source.strings.count("ogl_compress_tex") == 0);
    REQUIRE(source.strings.count("ogl_max_texsize") == 0);
}

TEST_CASE("render preferences preserve custom renderer names and unknown "
          "storage fallback") {
    Source source;
    source.strings["renderer"] = "custom-renderer";
    source.strings["software_compress_tex"] = "unknown";
    source.strings["ogl_compress_tex"] = "LZ4";
    REQUIRE(krkr::render::RendererName(source) == "custom-renderer");
    REQUIRE(krkr::render::SoftwareStorage(source) ==
            krkr::render::TextureStorage::Normal);
    REQUIRE(krkr::render::GLStorage(source) ==
            krkr::render::TextureStorage::Normal);
}

TEST_CASE(
    "render preferences retain fresh on-demand reads after source changes") {
    Source source;
    source.strings["software_compress_tex"] = "lz4";
    REQUIRE(krkr::render::SoftwareStorage(source) ==
            krkr::render::TextureStorage::Lz4);
    source.strings["software_compress_tex"] = "halfline";
    REQUIRE(krkr::render::SoftwareStorage(source) ==
            krkr::render::TextureStorage::HalfRows);
    source.strings["ogl_compress_tex"] = "half";
    REQUIRE(krkr::render::GLStorage(source) ==
            krkr::render::TextureStorage::Half);
    REQUIRE(source.reads.size() == 3);
}

TEST_CASE(
    "render texture limits retain low-device override and signed boundaries") {
    using krkr::render::TextureLimit;
    REQUIRE(TextureLimit(4096, 512) == 512);
    REQUIRE(TextureLimit(2048, 4096) == 2048);
    REQUIRE(TextureLimit(1024, 2048) == 1024);
    REQUIRE(TextureLimit(1023, 2048) == 2048);
    REQUIRE(TextureLimit(0, 4096) == 4096);
    REQUIRE(TextureLimit(4096, 0) == 4096);
    REQUIRE(TextureLimit(4096, -1) == 4096);
    REQUIRE(TextureLimit(512, std::numeric_limits<int>::max()) ==
            static_cast<unsigned>(std::numeric_limits<int>::max()));
    REQUIRE(TextureLimit(std::numeric_limits<unsigned>::max(),
                         std::numeric_limits<int>::max()) ==
            static_cast<unsigned>(std::numeric_limits<int>::max()));
}
