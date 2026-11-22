#include <catch2/catch_test_macros.hpp>
#include <cocos/2d/CCNode.h>
#include <cocos/base/CCScheduler.h>

#include "cocos2d/CocosVideoPresentation.h"

TEST_CASE("cocos window video presentation detaches and cancels callbacks",
          "[cocos][video]") {
    cocos2d::Node parent;
    cocos2d::Node root;
    parent.addChild(&root);
    auto presentation = krkr::video::CreateCocosVideoPresentation(root);

    presentation->SetVisible(false);
    REQUIRE_FALSE(root.isVisible());
    presentation->SetPictureVisible(
        true); // No GL sprite before the first frame.
    presentation->SetVisible(true);
    REQUIRE(root.isVisible());

    root.schedule([](float) {}, "update video");
    REQUIRE(root.getScheduler()->isScheduled("update video", &root));
    presentation.reset();
    REQUIRE(parent.getChildren().empty());
    REQUIRE_FALSE(root.getScheduler()->isScheduled("update video", &root));
}

TEST_CASE("cocos window video presentation handles a detached root",
          "[cocos][video]") {
    cocos2d::Node parent;
    cocos2d::Node root;
    parent.addChild(&root);
    auto presentation = krkr::video::CreateCocosVideoPresentation(root);

    // The window can detach the node before its player is released.
    root.removeFromParentAndCleanup(false);
    root.schedule([](float) {}, "update video");
    REQUIRE(root.getScheduler()->isScheduled("update video", &root));

    presentation.reset();
    REQUIRE_FALSE(root.getScheduler()->isScheduled("update video", &root));
}
