#include "HostFrame.h"

#include <cocos2d.h>
#include "FrameStep.h"
#include "Platform.h"

int TVPDrawSceneOnce(int interval) {
    static krkr::event::ModalFramePacer pacer;
    return pacer.DrawIfDue(
        TVPGetRoughTickCount32(), interval,
        [] { cocos2d::Director::getInstance()->drawScene(); },
        TVPForceSwapBuffer);
}
