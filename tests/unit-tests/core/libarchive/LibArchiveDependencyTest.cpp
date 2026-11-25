#include <catch2/catch_test_macros.hpp>
#include <archive.h>
#include <archive_entry.h>

#include <array>
#include <memory>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>
#include <string>

TEST_CASE("libarchive imported target preserves compression and XML closure") {
    using Archive = std::unique_ptr<archive, decltype(&archive_read_free)>;
    using Writer = std::unique_ptr<archive, decltype(&archive_write_free)>;
    const std::array filters{ archive_write_add_filter_gzip,
                              archive_write_add_filter_bzip2,
                              archive_write_add_filter_xz,
                              archive_write_add_filter_lz4,
                              archive_write_add_filter_zstd };
    const std::string payload = "archive dependency contract 中文\n";
    for(std::size_t filter = 0; filter < filters.size(); ++filter) {
        DYNAMIC_SECTION("compression filter " << filter) {
            std::array<char, 32768> bytes{};
            std::size_t used = 0;
            Writer writer(archive_write_new(), archive_write_free);
            REQUIRE(writer);
            REQUIRE(filters[filter](writer.get()) == ARCHIVE_OK);
            REQUIRE(archive_write_set_format_pax_restricted(writer.get()) ==
                    ARCHIVE_OK);
            REQUIRE(archive_write_open_memory(writer.get(), bytes.data(),
                                              bytes.size(),
                                              &used) == ARCHIVE_OK);
            std::unique_ptr<archive_entry, decltype(&archive_entry_free)> entry(
                archive_entry_new(), archive_entry_free);
            archive_entry_set_pathname(entry.get(), "fixture.txt");
            archive_entry_set_size(entry.get(), payload.size());
            archive_entry_set_filetype(entry.get(), AE_IFREG);
            archive_entry_set_perm(entry.get(), 0644);
            REQUIRE(archive_write_header(writer.get(), entry.get()) ==
                    ARCHIVE_OK);
            REQUIRE(archive_write_data(writer.get(), payload.data(),
                                       payload.size()) ==
                    static_cast<la_ssize_t>(payload.size()));
            REQUIRE(archive_write_close(writer.get()) == ARCHIVE_OK);
            REQUIRE(used > 0);
            Archive reader(archive_read_new(), archive_read_free);
            // Register all formats, pulling the XAR XML reader into this
            // standalone static consumer even though the fixture is a TAR.
            REQUIRE(archive_read_support_filter_all(reader.get()) ==
                    ARCHIVE_OK);
            REQUIRE(archive_read_support_format_all(reader.get()) ==
                    ARCHIVE_OK);
            REQUIRE(archive_read_open_memory(reader.get(), bytes.data(),
                                             used) == ARCHIVE_OK);
            archive_entry *restored = nullptr;
            REQUIRE(archive_read_next_header(reader.get(), &restored) ==
                    ARCHIVE_OK);
            REQUIRE(std::string(archive_entry_pathname(restored)) ==
                    "fixture.txt");
            std::string actual(payload.size(), '\0');
            REQUIRE(
                archive_read_data(reader.get(), actual.data(), actual.size()) ==
                static_cast<la_ssize_t>(actual.size()));
            REQUIRE(actual == payload);
            REQUIRE(archive_read_next_header(reader.get(), &restored) ==
                    ARCHIVE_EOF);
        }
    }

    // Windows CNG and Apple CommonCrypto provide the hashes XAR requires.
    // The current Linux overlay disables OpenSSL and has no hash backend;
    // verify that boundary explicitly instead of assuming XML implies XAR.
    Archive reader(archive_read_new(), archive_read_free);
    const int xarSupport = archive_read_support_format_xar(reader.get());
#if defined(_WIN32) || defined(__APPLE__)
    REQUIRE(xarSupport == ARCHIVE_OK);
#else
    REQUIRE((xarSupport == ARCHIVE_OK || xarSupport == ARCHIVE_WARN));
    if(xarSupport == ARCHIVE_WARN)
        REQUIRE(std::string(archive_error_string(reader.get()))
                    .find("Xar not supported") != std::string::npos);
#endif
    if(xarSupport == ARCHIVE_OK) {
        std::ifstream fixture(std::filesystem::u8path(KRKR2_XAR_FIXTURE),
                              std::ios::binary);
        REQUIRE(fixture.good());
        const std::vector<char> xar{ std::istreambuf_iterator<char>(fixture),
                                     {} };
        REQUIRE(archive_read_support_filter_all(reader.get()) == ARCHIVE_OK);
        const auto openResult =
            archive_read_open_memory(reader.get(), xar.data(), xar.size());
        const char *openError = archive_error_string(reader.get());
        INFO("XAR open result "
             << openResult << ": "
             << (openError ? openError : "no archive error"));
        REQUIRE(openResult == ARCHIVE_OK);
        archive_entry *entry = nullptr;
        REQUIRE(archive_read_next_header(reader.get(), &entry) == ARCHIVE_OK);
        REQUIRE(std::string(archive_entry_pathname(entry)) == "fixture.txt");
        std::string restored(payload.size(), '\0');
        REQUIRE(
            archive_read_data(reader.get(), restored.data(), restored.size()) ==
            static_cast<la_ssize_t>(restored.size()));
        REQUIRE(restored == payload);
        REQUIRE(archive_read_next_header(reader.get(), &entry) == ARCHIVE_EOF);
    }
}
