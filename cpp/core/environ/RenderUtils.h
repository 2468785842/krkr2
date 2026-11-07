//
// Created by LiDon on 2025/9/25.
//
#pragma once

#include "PostUpdateEvent.h"

// Draw at most once when the interval has elapsed; return remaining delay.
// The Cocos drawing and platform swap implementation is private to environ.
int TVPDrawSceneOnce(int interval);
