#pragma once

#include "VideoPresentation.h"

namespace cocos2d {
    class Node;
}

namespace krkr::video {
    // The preview drives presentation from its own update loop.
    std::unique_ptr<Presentation>
    CreateCocosVideoPresentation(cocos2d::Node &root);
} // namespace krkr::video
