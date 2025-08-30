#include "TipsHelpForm.h"
#include "ui/UIHelper.h"
#include "ui/UIListView.h"
#include "cocos2d/MainScene.h"
#include "csd/CsdUIFactory.h"

using namespace cocos2d;
using namespace cocos2d::ui;

TVPTipsHelpForm *TVPTipsHelpForm::create() {
    auto *ret = new TVPTipsHelpForm;
    ret->initFromFile(nullptr, Csd::createAllTips(), nullptr);
    ret->autorelease();
    return ret;
}

TVPTipsHelpForm *TVPTipsHelpForm::show(const char *tipName) {
    TVPTipsHelpForm *ui = create();
    if(tipName)
        ui->setOneTip(tipName);
    TVPMainScene::GetInstance()->pushUIForm(
        ui, TVPMainScene::eEnterAniOverFromRight);
    return ui;
}

void TVPTipsHelpForm::setOneTip(const std::string &tipName) {
    while(!_tipslist->getItems().empty()) {
        Node *cell = _tipslist->getItem(_tipslist->getItems().size() - 1);
        if(cell->getName() == tipName) {
            break;
        } else {
            _tipslist->removeLastItem();
        }
    }
    while(!_tipslist->getItems().empty()) {
        Node *cell = _tipslist->getItem(0);
        if(cell->getName() == tipName) {
            break;
        } else {
            _tipslist->removeItem(0);
        }
    }
}

void TVPTipsHelpForm::rearrangeLayout() {
    cocos2d::Size sceneSize = TVPMainScene::GetInstance()->getUINodeSize();
    cocos2d::Size rootSize = RootNode->getContentSize();
    float scale = sceneSize.width / rootSize.width;
    rootSize.height = rootSize.width * sceneSize.height / sceneSize.width;
    setContentSize(rootSize);
    setScale(scale);
    RootNode->setContentSize(rootSize);
    ui::Helper::doLayout(RootNode);
}

void TVPTipsHelpForm::bindBodyController(const Node *allNodes) {
    _tipslist = dynamic_cast<ListView *>(allNodes->getChildByName("tipslist"));
    auto *btn_close = allNodes->getChildByName<Widget *>("btn_close");
    btn_close->addClickEventListener([this](Ref *p) {
        dynamic_cast<Widget *>(p)->setEnabled(false);
        TVPMainScene::GetInstance()->popUIForm(this);
    });
    auto *nullCell = Widget::create();
    nullCell->setContentSize(Size(_tipslist->getContentSize().width, 200));
    _tipslist->pushBackCustomItem(nullCell);
}
