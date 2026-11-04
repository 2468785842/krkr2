#pragma once

#include "PointerCoordinates.h"
#include <cocos/2d/CCNode.h>

namespace krkr::cocosinput {
    inline input::PointerPixel layerPosition(cocos2d::Node &layer,
                                             const cocos2d::Vec2 &world) {
        const auto local = layer.convertToNodeSpace(world);
        return input::pointerPixel(input::flipPointerY(
            { local.x, local.y }, layer.getContentSize().height));
    }

    inline cocos2d::Vec2 cursorPosition(cocos2d::Node &layer,
                                        cocos2d::Node &cursorParent,
                                        input::PointerPixel position) {
        const auto local = input::flipPointerY(
            { static_cast<float>(position.x), static_cast<float>(position.y) },
            layer.getContentSize().height);
        const auto world = layer.convertToWorldSpace({ local.x, local.y });
        return cursorParent.convertToNodeSpace(world);
    }
} // namespace krkr::cocosinput
