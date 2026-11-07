#include <catch2/catch_test_macros.hpp>
#include <cocos/2d/CCNode.h>

namespace {
    cocos2d::Node *append(cocos2d::Node &parent, int z) {
        auto *child = new cocos2d::Node;
        parent.addChild(child, z);
        child->release(); // The parent now owns the only reference.
        return child;
    }
} // namespace

TEST_CASE("cocos window UI stays above a game added later", "[window][cocos]") {
    cocos2d::Node parent;
    auto *ui = append(parent, 4);
    auto *game = append(parent, 0);
    parent.sortAllChildren();
    REQUIRE(parent.getChildren().at(0) == game);
    REQUIRE(parent.getChildren().at(1) == ui);
}

TEST_CASE("cocos window order preserves negative z and arrival ties",
          "[window][cocos]") {
    cocos2d::Node parent;
    auto *first = append(parent, 0);
    auto *overlay = append(parent, 4);
    auto *negative = append(parent, -1);
    auto *second = append(parent, 0);
    parent.sortAllChildren();
    REQUIRE(parent.getChildren().at(0) == negative);
    REQUIRE(parent.getChildren().at(1) == first);
    REQUIRE(parent.getChildren().at(2) == second);
    REQUIRE(parent.getChildren().at(3) == overlay);
    first->setLocalZOrder(4);
    parent.sortAllChildren();
    REQUIRE(parent.getChildren().at(2) == overlay);
    REQUIRE(parent.getChildren().at(3) == first);
}
