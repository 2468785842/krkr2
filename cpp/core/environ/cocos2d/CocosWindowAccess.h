#pragma once

class iWindowLayer;
namespace cocos2d {
    class Node;
}

// Borrowed node owned by the Cocos window. Only Cocos media adapters may use
// it. A non-Cocos window is rejected with std::bad_cast, never silently
// ignored.
cocos2d::Node *TVPGetCocosWindowPrimaryArea(iWindowLayer &window);
