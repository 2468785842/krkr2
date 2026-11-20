#pragma once

class iTVPTexture2D;
namespace cocos2d {
    class Texture2D;
}

// Synchronously uploads CPU rows or borrows a GL name. The returned Cocos
// texture is autoreleased when newly created; a Sprite retains it as usual.
cocos2d::Texture2D *
TVPGetCocosPresentationTexture(iTVPTexture2D &source,
                               cocos2d::Texture2D *previous);
