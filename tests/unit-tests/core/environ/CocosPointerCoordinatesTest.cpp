#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "cocos2d/CocosPointerCoordinates.h"

namespace {
    cocos2d::Node *append(cocos2d::Node &parent) {
        auto *node = new cocos2d::Node;
        parent.addChild(node);
        node->release();
        return node;
    }
} // namespace

TEST_CASE("cocos pointer events keep primary layer coordinates after scaling",
          "[window][cocos]") {
    cocos2d::Node root;
    auto &layer = *append(root);
    layer.setContentSize({ 640, 480 });
    layer.setPosition({ 20, 30 });
    layer.setScale(2);
    root.setScale(.5f);
    root.setPosition({ 100, 50 });
    root.setRotation(30);
    layer.setAnchorPoint({ .5f, .5f });
    const auto world = layer.convertToWorldSpace({ 17.75f, 442.25f });
    const auto point = krkr::cocosinput::layerPosition(layer, world);
    REQUIRE(point.x == 17);
    REQUIRE(point.y == 37);
}

TEST_CASE("cocos pointer capture preserves positions beyond layer edges",
          "[window][cocos]") {
    cocos2d::Node layer;
    layer.setContentSize({ 640, 480 });
    layer.setPosition({ -50, 25 });
    layer.setScale(.5f);
    auto point = krkr::cocosinput::layerPosition(
        layer, layer.convertToWorldSpace({ -5.75f, -32.75f }));
    REQUIRE(point.x == -5);
    REQUIRE(point.y == 512);
}

TEST_CASE("cocos pointer cursor presentation is separate from script position",
          "[window][cocos]") {
    cocos2d::Node root;
    auto &layer = *append(root);
    auto &cursorParent = *append(root);
    layer.setContentSize({ 640, 480 });
    layer.setPosition({ 20, 30 });
    layer.setScale(2);
    cursorParent.setPosition({ -10, 5 });
    const krkr::input::PointerPixel requested{ 17, 37 };
    const auto presentation =
        krkr::cocosinput::cursorPosition(layer, cursorParent, requested);
    REQUIRE(presentation.x == Catch::Approx(64));
    REQUIRE(presentation.y == Catch::Approx(911));
    const auto roundtrip = krkr::cocosinput::layerPosition(
        layer, cursorParent.convertToWorldSpace(presentation));
    REQUIRE(roundtrip.x == requested.x);
    REQUIRE(roundtrip.y == requested.y);
    // The old setter cached window-parent coordinates rather than script
    // coordinates. A cursor can also have a different parent of its own.
    const auto oldCache =
        root.convertToNodeSpace(layer.convertToWorldSpace({ 17, 480 - 37 }));
    REQUIRE(oldCache.x == Catch::Approx(54));
    REQUIRE(oldCache.y == Catch::Approx(916));
    REQUIRE(presentation.x != requested.x);
    REQUIRE(presentation.y != requested.y);
}
