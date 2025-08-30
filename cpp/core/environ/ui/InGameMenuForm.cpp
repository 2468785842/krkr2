#include "InGameMenuForm.h"
#include "cocos2d/MainScene.h"
#include "ui/UIButton.h"
#include "ui/UIListView.h"
#include "ui/UIText.h"
#include "MenuItemIntf.h"
#include "ui/UIHelper.h"
#include "tjsGlobalStringMap.h"
#include "csd/CsdUIFactory.h"

using namespace cocos2d;
using namespace cocos2d::ui;

TVPInGameMenuForm *TVPInGameMenuForm::create(const std::string &title,
                                             tTJSNI_MenuItem *item) {
    auto *ret = new TVPInGameMenuForm;
    ret->autorelease();
    ret->initFromFile(Csd::createNaviBar(), Csd::createListView(), nullptr);
    ret->initMenu(title, item);
    return ret;
}

void TVPInGameMenuForm::bindHeaderController(const Node *allNodes) {
    _title = allNodes->getChildByName<Button *>("title");
    if(_title)
        _title->setEnabled(false);
}

void TVPInGameMenuForm::bindBodyController(const Node *allNodes) {
    _list = allNodes->getChildByName<ListView *>("list");
    if(NaviBar.Left) {
        NaviBar.Left->addClickEventListener([this](cocos2d::Ref *) {
            TVPMainScene::GetInstance()->popUIForm(this);
        });
    }
}

void TVPInGameMenuForm::initMenu(const std::string &title,
                                 tTJSNI_MenuItem *item) {
    _list->removeAllItems();
    if(_title) {
        if(title.empty()) {
            ttstr caption;
            item->GetCaption(caption);
            _title->setTitleText(caption.AsStdString());
        } else {
            _title->setTitleText(title);
        }
    }

    int count = item->GetChildren().size();
    int idx = 0;
    ttstr seperator = TJS::TJSMapGlobalStringMap(TJS_W("-"));
    for(int i = 0; i < count; ++i) {
        tTJSNI_MenuItem *subitem =
            static_cast<tTJSNI_MenuItem *>(item->GetChildren().at(i));
        ttstr caption;
        subitem->GetCaption(caption);
        if(caption.IsEmpty() || caption == TJS_W("+"))
            continue;
        _list->pushBackCustomItem(
            createMenuItem(idx, subitem, caption.AsStdString()));
        if(caption != seperator)
            ++idx;
    }
}

cocos2d::ui::Widget *
TVPInGameMenuForm::createMenuItem(int idx, tTJSNI_MenuItem *item,
                                  const std::string &caption) {
    iPreferenceItem *ret = nullptr;
    const cocos2d::Size &size = _list->getContentSize();
    if(!item->GetChildren().empty()) {
        ret = CreatePreferenceItem<tPreferenceItemSubDir>(idx, size, caption);
        ret->addClickEventListener([=](Ref *) {
            TVPMainScene::GetInstance()->pushUIForm(create(caption, item));
        });
    } else if(item->GetGroup() > 0 || item->GetRadio()) {
        auto getter = [=]() -> bool { return item->GetChecked(); };
        auto setter = [=](bool) {
            item->OnClick();
            TVPMainScene::GetInstance()->popAllUIForm();
        };
        ret = CreatePreferenceItem<tPreferenceItemCheckBox>(
            idx, size, caption, [=](tPreferenceItemCheckBox *item) {
                item->_getter = getter;
                item->_setter = setter;
            });
    } else if(item->GetChecked()) {
        auto getter = [=]() -> bool { return item->GetChecked(); };
        auto setter = [=](bool) { item->OnClick(); };
        ret = CreatePreferenceItem<tPreferenceItemCheckBox>(
            idx, size, caption, [=](tPreferenceItemCheckBox *item) {
                item->_getter = getter;
                item->_setter = setter;
            });
    } else if(caption == "-") {
        float w = size.width;
        Widget *sep =
            Csd::createSeperateItem(w, 2.0f, Color4F(0.6f, 0.6f, 0.6f, 1.0f));
        return sep;
    } else {
        ret = CreatePreferenceItem<tPreferenceItemConstant>(idx, size, caption);
        ret->addClickEventListener([=](Ref *) {
            TVPMainScene::GetInstance()->scheduleOnce(
                [c = TVPMainScene::GetInstance()](float) { c->popAllUIForm(); },
                0, "close_menu");
            item->OnClick();
        });
        ret->setTouchEnabled(true);
    }
    return ret;
}

void TVPShowPopMenu(tTJSNI_MenuItem *menu) {
    TVPMainScene::GetInstance()->pushUIForm(
        TVPInGameMenuForm::create(std::string(), menu));
}
