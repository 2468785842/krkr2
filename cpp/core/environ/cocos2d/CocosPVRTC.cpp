#include "RenderHost.h"

#include <cocos/base/pvr.h>

namespace krkr::renderhost {
    void DecodePVRTC(const void *compressed, int width, int height,
                     void *pixels, bool twoBitsPerPixel) {
        PVRTDecompressPVRTC(compressed, width, height, pixels, twoBitsPerPixel);
    }
} // namespace krkr::renderhost
