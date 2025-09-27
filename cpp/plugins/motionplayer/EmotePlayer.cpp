//
// Created by LiDon on 2025/9/15.
//

#include "EmotePlayer.h"

#include "ncbind.hpp"

emote::EmotePlayer::EmotePlayer(ResourceManager resManager) :
    _resManager(resManager), _useD3D(false), _maskMode(MaskModeType::Alpha),
    _completionType(stNearest) {
}


void emote::EmotePlayer::initPhysics(tTJSVariant rule) {
    // "metadata"

}

tTJSVariant emote::EmotePlayer::getDiffTimelineLabelList() const {
    return getTimelineLabelList([](int diff) -> bool { return diff != 0; });
}

tTJSVariant emote::EmotePlayer::getMainTimelineLabelList() const {
    return getTimelineLabelList([](int diff) -> bool { return diff == 0; });
}

tTJSVariant emote::EmotePlayer::getTimelineLabelList(const std::function<bool(int)> &diffFilter) const {
    iTJSDispatch2 *array = TJSCreateArrayObject();
    auto objs = this->_resManager.getPSBFile()->getObjects();
    auto metadata = std::dynamic_pointer_cast<PSB::PSBDictionary>((*objs)["metadata"]);
    auto timelineControls = std::dynamic_pointer_cast<PSB::PSBList>((*metadata)["timelineControl"]);
    for(auto v : *timelineControls) {
        auto timelineControl = std::dynamic_pointer_cast<PSB::PSBDictionary>(v);
        auto diff = std::dynamic_pointer_cast<PSB::PSBNumber>((*timelineControl)["diff"]);

        if(diffFilter(!diff ? 0 : diff->getValue<int>())) continue;

        auto label = (*timelineControl)["label"]->toTJSVal();
        tTJSVariant *args[] = { &label };
        static tjs_uint addHint = 0;
        array->FuncCall(0, TJS_W("add"), &addHint, nullptr, 1, args, array);
    }

    tTJSVariant result(array, array);
    array->Release();
    return result;
}