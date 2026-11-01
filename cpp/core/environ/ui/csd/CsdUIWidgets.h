#pragma once

#include <cocos/ui/CocosGUI.h>

namespace Csd {

    cocos2d::ui::Widget *createListItem(const cocos2d::Size &size, float scale);
    cocos2d::ui::Widget *createSelectListItem(const cocos2d::Size &size,
                                              float scale);
    cocos2d::ui::Widget *createComctrlSelectListItem(const cocos2d::Size &size,
                                                     float scale);
    cocos2d::ui::Widget *createCheckBoxItem(const cocos2d::Size &size,
                                            float scale);
    cocos2d::ui::Widget *createSubDirItem(const cocos2d::Size &size,
                                          float scale);
    cocos2d::ui::Widget *createSliderIconItem(const cocos2d::Size &size,
                                              float scale);
    cocos2d::ui::Widget *createSliderTextItem(const cocos2d::Size &size,
                                              float scale);
    cocos2d::ui::Widget *createDeletableItem(const cocos2d::Size &size,
                                             float scale);
    cocos2d::ui::Widget *createFileManageMenu(const cocos2d::Size &size,
                                              float scale);
    cocos2d::ui::Widget *createListViewPopup(const cocos2d::Size &size,
                                             float scale);
    cocos2d::ui::Widget *createGameMenu(const cocos2d::Size &size, float scale);
    cocos2d::ui::Widget *createMenuList(const cocos2d::Size &size, float scale);
    cocos2d::ui::Widget *createRecentListItem(const cocos2d::Size &size,
                                              float scale);

} // namespace Csd
