//
// Created by LiDon on 2025/9/15.
//

#include "EmotePlayer.h"

emote::EmotePlayer::EmotePlayer(ResourceManager resManager) :
    _resManager(resManager), _useD3D(false), _maskMode(MaskModeType::Alpha),
    _completionType(stNearest) {
}


void emote::EmotePlayer::initPhysics(tTJSVariant rule) {}

tTJSVariant emote::EmotePlayer::getMainTimelineLabelList() {
    // TODO:
    iTJSDispatch2 *array = TJSCreateArrayObject();

    // 0:sample_全自動_test
    // 1:sample_全自動_test2
    // 2:sample_00
    // 3:sample_01
    // 4:sample_02
    // 5:sample_03
    // 6:sample_04
    // 7:sample_05
    // 8:首横振り
    // 9:挨拶
    // 10:ピョン
    // 11:下ピョン
    // 12:びっくり
    // 13:困り
    return array;
}