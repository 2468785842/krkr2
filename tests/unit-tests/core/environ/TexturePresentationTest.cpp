#include "TexturePresentation.h"
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <string>
#include <type_traits>
#include <vector>

using namespace krkr::render;

TEST_CASE("presentation CPU rows preserve padding and signed pitch") {
    std::array<std::uint8_t, 24> bytes{};
    RGBAFrame frame{ 2, 2, bytes.data(), 12 };
    REQUIRE(frame.Row(0) == bytes.data());
    REQUIRE(frame.Row(1) == bytes.data() + 12);
    frame.pixels = bytes.data() + 12;
    frame.pitch = -12;
    REQUIRE(frame.Row(0) == bytes.data() + 12);
    REQUIRE(frame.Row(1) == bytes.data());
}

TEST_CASE("presentation compressed rows borrow the physical row reader") {
    std::array<std::uint8_t, 8> a{}, b{};
    const std::array<const void *, 3> rows{ a.data(), b.data(), a.data() };
    RGBAFrame frame{ 2,
                     3,
                     nullptr,
                     8,
                     &rows,
                     [](const void *context, unsigned y) -> const void * {
                         return (*static_cast<decltype(rows) *>(context))[y];
                     } };
    TextureFrame descriptor = frame;
    const auto &cpu = std::get<RGBAFrame>(descriptor);
    REQUIRE(cpu.Row(0) == a.data());
    REQUIRE(cpu.Row(1) == b.data());
    REQUIRE(cpu.Row(2) == a.data());
}

namespace {
    struct Owner {
        int references = 1;
        int retains = 0;
        Owner *dependent = nullptr;
        std::string name;
        std::vector<std::string> &events;

        void AddRef() {
            REQUIRE(references > 0);
            ++references;
            ++retains;
            events.push_back("retain:" + name);
        }
        void Release() {
            REQUIRE(references > 0);
            --references;
            events.push_back("release:" + name);
            if(!references && dependent)
                dependent->Release();
        }
    };
} // namespace

TEST_CASE("presentation lease retains a replacement before its owner dies") {
    std::vector<std::string> events;
    Owner replacement{ 1, 0, nullptr, "replacement", events };
    Owner previous{ 1, 0, &replacement, "previous", events };
    TextureLease lease;
    lease.Reset(previous);
    previous.Release();
    events.clear();
    lease.Reset(replacement);
    REQUIRE(events ==
            std::vector<std::string>{ "retain:replacement", "release:previous",
                                      "release:replacement" });
    REQUIRE(previous.references == 0);
    REQUIRE(replacement.references == 1);
    lease.Reset();
    REQUIRE(replacement.references == 0);
}

TEST_CASE("presentation lease reuses a source and releases it exactly once") {
    STATIC_REQUIRE(!std::is_copy_constructible_v<TextureLease>);
    std::vector<std::string> events;
    Owner source{ 1, 0, nullptr, "source", events };
    {
        TextureLease lease;
        lease.Reset(source);
        lease.Reset(source);
        REQUIRE(source.retains == 1);
        REQUIRE(source.references == 2);
        lease.Reset();
        lease.Reset();
        REQUIRE(source.references == 1);
        lease.Reset(source);
    }
    REQUIRE(source.references == 1);
    REQUIRE(source.retains == 2);
}
