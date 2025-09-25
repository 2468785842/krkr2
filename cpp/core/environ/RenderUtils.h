//
// Created by LiDon on 2025/9/25.
//
#pragma once

#include <cocos2d.h>
#include "Platform.h"

static inline void (*_postUpdate)() = nullptr;

inline void TVPSetPostUpdateEvent(void (*f)()) { _postUpdate = f; }

inline int TVPDrawSceneOnce(int interval) {
    static tjs_uint64 lastTick = TVPGetRoughTickCount32();
    tjs_uint64 curTick = TVPGetRoughTickCount32();
    int remain = interval - (curTick - lastTick);
    if(remain <= 0) {
        if(_postUpdate)
            _postUpdate();
        auto *director = cocos2d::Director::getInstance();
        director->drawScene();
        TVPForceSwapBuffer();
        lastTick = curTick;
        return 0;
    }
    return remain;
}