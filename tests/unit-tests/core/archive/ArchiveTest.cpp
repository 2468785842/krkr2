#include <catch2/catch_test_macros.hpp>

#include "HeadlessRuntime.h"
#if defined(KRKR2_ARCHIVE_TEST_XP3)
#include "archive/xp3/XP3Archive.h"
#endif
#include "storage/impl/ArchiveFilename.h"
#include "storage/impl/ArchiveHandleCache.h"
#include "storage/impl/ArchiveRegistry.h"

#include <memory>
#include <stdexcept>
#include <string>

extern tTVPArchive *TVPOpenZIPArchive(const ttstr &, tTJSBinaryStream *, bool);
extern tTVPArchive *TVPOpenTARArchive(const ttstr &, tTJSBinaryStream *, bool);

namespace {
    struct Registry {
        Registry() {
            TVPClearArchiveCreators();
            TVPRegisterArchiveCreator(TVPOpenZIPArchive);
        }
        ~Registry() { TVPClearArchiveCreators(); }
    };

    using Archive = std::unique_ptr<tTVPArchive, void (*)(tTVPArchive *)>;

    Archive open(const char *filename) {
        const std::string name =
            std::string(KRKR2_ARCHIVE_TEST_DATA) + "/" + filename;
        return { TVPOpenArchive(ttstr(name), true), [](tTVPArchive *archive) {
                    if(archive)
                        archive->Release();
                } };
    }

    std::string read(tTJSBinaryStream &stream) {
        std::string text(stream.GetSize(), '\0');
        stream.ReadBuffer(text.data(), static_cast<tjs_uint>(text.size()));
        return text;
    }
} // namespace

TEST_CASE("ZIP enumerates normalized UTF-8 names and reads stored and deflated "
          "entries",
          "[archive]") {
    Registry registry;
    auto archive = open("mixed.zip");
    REQUIRE(archive);
    REQUIRE(archive->GetCount() == 4);
    REQUIRE(archive->IsExistent(TJS_W("dir/stored.txt")));
    REQUIRE(archive->IsExistent(TJS_W("日本語.txt")));
    std::unique_ptr<tTJSBinaryStream> stored{ archive->CreateStream(
        TJS_W("dir/stored.txt")) };
    REQUIRE(read(*stored) == "0123456789");
    std::unique_ptr<tTJSBinaryStream> unicode{ archive->CreateStream(
        TJS_W("日本語.txt")) };
    REQUIRE(read(*unicode) == "中文と日本語");
    std::unique_ptr<tTJSBinaryStream> deflated{ archive->CreateStream(
        TJS_W("deflated.txt")) };
    std::string expected;
    for(int i = 0; i < 64; ++i)
        expected += "compressed-data-";
    REQUIRE(read(*deflated) == expected);
    REQUIRE(deflated->Seek(-5, TJS_BS_SEEK_END) == expected.size() - 5);
    char end[5];
    REQUIRE(deflated->Read(end, sizeof(end)) == 5);
    REQUIRE(std::string(end, sizeof(end)) == "data-");
    std::unique_ptr<tTJSBinaryStream> empty{ archive->CreateStream(
        TJS_W("empty.bin")) };
    REQUIRE(empty->GetSize() == 0);
}

TEST_CASE(
    "ZIP stored streams keep the archive alive and have independent positions",
    "[archive]") {
    Registry registry;
    auto archive = open("mixed.zip");
    REQUIRE(archive);
    std::unique_ptr<tTJSBinaryStream> first{ archive->CreateStream(
        TJS_W("dir/stored.txt")) };
    std::unique_ptr<tTJSBinaryStream> second{ archive->CreateStream(
        TJS_W("dir/stored.txt")) };
    archive.reset();
    REQUIRE(first->Seek(4, TJS_BS_SEEK_SET) == 4);
    REQUIRE(read(*second) == "0123456789");
    char text[3];
    REQUIRE(first->Read(text, sizeof(text)) == 3);
    REQUIRE(std::string(text, sizeof(text)) == "456");
    first.reset();
    REQUIRE(TestLiveArchiveStreams() > 0);
    second.reset();
    REQUIRE(TestLiveArchiveStreams() == 0);
}

TEST_CASE("ZIP64 local headers remain supported without cocos", "[archive]") {
    Registry registry;
    auto archive = open("zip64.zip");
    REQUIRE(archive);
    REQUIRE(archive->GetCount() == 1);
    std::unique_ptr<tTJSBinaryStream> stream{ archive->CreateStream(
        TJS_W("zip64.bin")) };
    REQUIRE(read(*stream) == "zip64 local header");
}

TEST_CASE("Archive registry closes rejected and throwing input streams",
          "[archive]") {
    Registry registry;
    REQUIRE_FALSE(open("invalid.zip"));
    REQUIRE_FALSE(open("truncated.zip"));
    REQUIRE_THROWS(open("invalid-name.zip"));
    REQUIRE(TestLiveArchiveStreams() == 0);
    TVPClearArchiveCreators();
    TVPRegisterArchiveCreator(
        [](const ttstr &, tTJSBinaryStream *stream, bool) -> tTVPArchive * {
            char byte;
            stream->ReadBuffer(&byte, 1);
            return nullptr;
        });
    TVPRegisterArchiveCreator(
        [](const ttstr &, tTJSBinaryStream *stream, bool) -> tTVPArchive * {
            REQUIRE(stream->GetPosition() == 0);
            throw std::runtime_error("rejected archive");
        });
    REQUIRE_THROWS_AS(open("mixed.zip"), std::runtime_error);
    REQUIRE(TestLiveArchiveStreams() == 0);
}

TEST_CASE("Archive filenames reject invalid UTF-8 and report the error",
          "[archive]") {
    const int before = TestArchiveFilenameErrors();
    ttstr filename;
    REQUIRE_THROWS(storeFilename(filename, "\xff", TJS_W("invalid.zip")));
    REQUIRE(TestArchiveFilenameErrors() == before + 1);
}

TEST_CASE("Archive handles returned after shutdown are closed", "[archive]") {
    int owner;
    const ttstr name(std::string(KRKR2_ARCHIVE_TEST_DATA) + "/mixed.zip");
    tTJSBinaryStream *stream = TVPGetCachedArchiveHandle(&owner, name);
    REQUIRE(TestLiveArchiveStreams() == 1);
    TVPShutdownArchiveHandleCache();
    TVPReleaseCachedArchiveHandle(&owner, stream);
    REQUIRE(TestLiveArchiveStreams() == 0);
}

TEST_CASE("TAR reads through shared archive handles without XP3", "[archive]") {
    Registry registry;
    TVPClearArchiveCreators();
    TVPRegisterArchiveCreator(TVPOpenTARArchive);
    REQUIRE_FALSE(open("invalid.zip"));
    auto archive = open("plain.tar");
    REQUIRE(archive);
    REQUIRE(archive->GetCount() == 1);
    std::unique_ptr<tTJSBinaryStream> stream{ archive->CreateStream(
        TJS_W("folder/readme.txt")) };
    archive.reset();
    REQUIRE(read(*stream) == "0123456789");
    stream.reset();
    REQUIRE(TestLiveArchiveStreams() == 0);
}

#if defined(KRKR2_ARCHIVE_TEST_XP3)
TEST_CASE("XP3 registry exceptions retain caller ownership until unwinding",
          "[archive]") {
    Registry registry;
    TVPClearArchiveCreators();
    TVPRegisterArchiveCreator(tTVPXP3Archive::Create);
    REQUIRE_THROWS(open("invalid-index.xp3"));
    REQUIRE(TestLiveArchiveStreams() == 0);
    const ttstr name(std::string(KRKR2_ARCHIVE_TEST_DATA) +
                     "/invalid-index.xp3");
    REQUIRE_THROWS(tTVPXP3Archive::Create(name));
    REQUIRE(TestLiveArchiveStreams() == 0);
}
#endif
