#include "RenderUtils.h"

#include <cocos2d.h>
#include "Platform.h"

int TVPDrawSceneOnce(int interval) {
    static tjs_uint64 lastTick = TVPGetRoughTickCount32();
    tjs_uint64 curTick = TVPGetRoughTickCount32();
    int remain = interval - (curTick - lastTick);
    if(remain <= 0) {
        TVPInvokePostUpdateEvent();
        cocos2d::Director::getInstance()->drawScene();
        TVPForceSwapBuffer();
        lastTick = curTick;
        return 0;
    }
    return remain;
}
