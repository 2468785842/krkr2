#pragma once

#include <string>

namespace cocos2d::ui {
    class Text;
    class Button;
} // namespace cocos2d::ui

bool TVPInitLocaleText(cocos2d::ui::Text *ctrl);
bool TVPInitLocaleText(cocos2d::ui::Button *ctrl);
bool TVPInitLocaleText(cocos2d::ui::Text *ctrl, const std::string &tid);
bool TVPInitLocaleText(cocos2d::ui::Button *ctrl, const std::string &tid);
