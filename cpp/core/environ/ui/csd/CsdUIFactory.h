//
// Created by lidong on 25-6-19.
//
#pragma once

#include <functional>

#include <cocos/ui/CocosGUI.h>

namespace Csd {

    using NodeBuilderFn = std::function<cocos2d::ui::Widget *(
        const cocos2d::Size &size, float scale)>;

    cocos2d::ui::Widget *createEmpty(const cocos2d::Size &size, float scale);

    cocos2d::ui::Widget *createMainFileSelector(const cocos2d::Size &size,
                                                float scale);
    cocos2d::ui::Widget *createTableView(const cocos2d::Size &size,
                                         float scale);
    cocos2d::ui::Widget *createNaviBarWithMenu(const cocos2d::Size &size,
                                               float scale);
    cocos2d::ui::Widget *createFileItem(const cocos2d::Size &size, float scale);
    cocos2d::ui::Widget *createNaviBar(const cocos2d::Size &size, float scale);
    cocos2d::ui::Widget *createBottomBarTextInput(const cocos2d::Size &size,
                                                  float scale);
    cocos2d::ui::Widget *createTextPairInput(const cocos2d::Size &size,
                                             float scale);
    cocos2d::ui::Widget *createSelectList(const cocos2d::Size &size,
                                          float scale);
    cocos2d::ui::Widget *createListView(const cocos2d::Size &size, float scale);
    cocos2d::ui::Widget *createMessageBox(const cocos2d::Size &size,
                                          float scale);
    cocos2d::ui::Widget *createProgressBox(const cocos2d::Size &size,
                                           float scale);
    cocos2d::ui::Widget *createCheckListDialog(const cocos2d::Size &size,
                                               float scale);
    cocos2d::ui::Widget *createMediaPlayerNavi(const cocos2d::Size &size,
                                               float scale);
    cocos2d::ui::Widget *createMediaPlayerBody(const cocos2d::Size &size,
                                               float scale);
    cocos2d::ui::Widget *createMediaPlayerFoot(const cocos2d::Size &size,
                                               float scale);
    cocos2d::ui::Widget *createAllTips(const cocos2d::Size &size, float scale);
    cocos2d::ui::Widget *createWinMgrOverlay(const cocos2d::Size &size,
                                             float scale);
    cocos2d::ui::Widget *createSeperateItem(float width, float lineHeight,
                                            const cocos2d::Color4F &color);

} // namespace Csd

#include "CsdUIWidgets.h"
