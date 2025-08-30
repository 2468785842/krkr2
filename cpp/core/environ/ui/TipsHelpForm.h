#pragma once
#include "BaseForm.h"

class TVPTipsHelpForm : public iTVPBaseForm {

    void bindHeaderController(const Node *allNodes) override {}
    void bindBodyController(const Node *allNodes) override;
    void bindFooterController(const Node *allNodes) override {}

    cocos2d::ui::ListView *_tipslist{};

public:
    static TVPTipsHelpForm *create();
    static TVPTipsHelpForm *show(const char *tipName = nullptr);

    void setOneTip(const std::string &tipName);
    void rearrangeLayout() override;
};