#include "RenderHost.h"
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <array>
#include <vector>

TEST_CASE(
    "PVRTC decoder retains transparent and opaque endpoints in both modes",
    "[image]") {
    for(bool twoBits : { false, true }) {
        const int width = twoBits ? 16 : 8;
        const int height = 8;
        for(unsigned char endpoint : { 0, 255 }) {
            std::array<unsigned char, 32> compressed;
            compressed.fill(endpoint);
            std::vector<unsigned char> pixels(width * height * 4 + 16, 0x5a);
            krkr::renderhost::DecodePVRTC(compressed.data(), width, height,
                                          pixels.data() + 8, twoBits);
            REQUIRE(std::all_of(pixels.begin(), pixels.begin() + 8,
                                [](auto byte) { return byte == 0x5a; }));
            REQUIRE(std::all_of(pixels.end() - 8, pixels.end(),
                                [](auto byte) { return byte == 0x5a; }));
            REQUIRE(std::all_of(
                pixels.begin() + 8, pixels.end() - 8,
                [endpoint](auto byte) { return byte == endpoint; }));
        }
    }
}
