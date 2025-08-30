#pragma once
#include "PreferenceForm.h"

class tTJSNI_MenuItem;

class TVPInGameMenuForm : public iTVPBaseForm {
public:
    static TVPInGameMenuForm *create(const std::string &title,
                                     tTJSNI_MenuItem *item);

    void bindHeaderController(const Node *allNodes) override;
    void bindBodyController(const Node *allNodes) override;
    void bindFooterController(const Node *allNodes) override {}

    void initMenu(const std::string &title, tTJSNI_MenuItem *item);

private:
    cocos2d::ui::Widget *createMenuItem(int idx, tTJSNI_MenuItem *item,
                                        const std::string &caption);

    cocos2d::ui::ListView *_list{};
    cocos2d::ui::Button *_title{};
};