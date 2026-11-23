#include "CocosLocale.h"

#include "ConfigManager/LocaleConfigManager.h"
#include "ui/UIText.h"
#include "ui/UIButton.h"

bool TVPInitLocaleText(cocos2d::ui::Text *ctrl) {
    if(!ctrl)
        return false;
    return TVPInitLocaleText(ctrl, ctrl->getString());
}

bool TVPInitLocaleText(cocos2d::ui::Button *ctrl) {
    if(!ctrl)
        return false;
    return TVPInitLocaleText(ctrl, ctrl->getTitleText());
}

bool TVPInitLocaleText(cocos2d::ui::Text *ctrl, const std::string &tid) {
    if(!ctrl)
        return false;

    std::string txt = LocaleConfigManager::GetInstance()->GetText(tid);
    if(txt.empty()) {
        ctrl->setString(tid);
        ctrl->setColor(cocos2d::Color3B::RED);
        return false;
    }

    ctrl->setString(txt);
    return true;
}

bool TVPInitLocaleText(cocos2d::ui::Button *ctrl, const std::string &tid) {
    if(!ctrl)
        return false;

    std::string txt = LocaleConfigManager::GetInstance()->GetText(tid);
    if(txt.empty()) {
        ctrl->setTitleText(tid);
        ctrl->setTitleColor(cocos2d::Color3B::RED);
        return false;
    }

    ctrl->setTitleText(txt);
    return true;
}
