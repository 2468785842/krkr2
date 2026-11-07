#include <catch2/catch_test_macros.hpp>
#include "InputCancellation.h"

TEST_CASE("input cancellation keeps keyboard and pointer releases separate",
          "[input]") {
    const auto plan = krkr::input::planInputCancellation(
        { 1, 2, 4, 13, 16, 27, 32, 65, 0x1c0, 0x1c1 }, {});
    REQUIRE(plan.pointerUps == std::vector<std::uint32_t>{ 1, 2, 4 });
    REQUIRE(plan.keyUps ==
            std::vector<std::uint32_t>{ 13, 16, 27, 32, 65, 0x1c0, 0x1c1 });
}

TEST_CASE("input cancellation filters mouse keys without fabricating presses",
          "[input]") {
    // Explicit legacy VK values check the script ABI independently of the
    // adapter.
    for(auto key : { 13u, 32u, 0x1c0u })
        REQUIRE(krkr::input::mouseKeyButton(key) == 1);
    for(auto key : { 27u, 0x1c1u })
        REQUIRE(krkr::input::mouseKeyButton(key) == 2);
    for(auto key : { 16u, 37u, 65u, 0x1b5u })
        REQUIRE(krkr::input::mouseKeyButton(key) == 0);
    // A key outside the layer never became a mouse down. It needs no mouse up.
    const auto plan = krkr::input::planInputCancellation(
        { 13, 16, 27, 32, 37, 65, 0x1b5, 0x1c0, 0x1c1 },
        { true, false, false });
    REQUIRE(plan.pointerUps.empty());
    REQUIRE(plan.keyUps == std::vector<std::uint32_t>{ 16, 37, 65, 0x1b5 });
}

TEST_CASE("input cancellation merges physical and emulated pointer releases",
          "[input]") {
    for(unsigned combination = 0; combination < 32; ++combination) {
        std::vector<std::uint32_t> released;
        if(combination & 1)
            released.push_back(1);
        if(combination & 2)
            released.push_back(2);
        if(combination & 4)
            released.push_back(4);
        const auto plan = krkr::input::planInputCancellation(
            released,
            { true, (combination & 8) != 0, (combination & 16) != 0 });
        std::vector<std::uint32_t> expected;
        if(combination & 9)
            expected.push_back(1);
        if(combination & 18)
            expected.push_back(2);
        if(combination & 4)
            expected.push_back(4);
        REQUIRE(plan.pointerUps == expected);
        REQUIRE(plan.keyUps.empty());
    }
}
