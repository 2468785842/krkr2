#include "BaseForm.h"
#include "cocos2d.h"
#include "cocostudio/ActionTimeline/CSLoader.h"
#include "Application.h"
#include "ui/UIWidget.h"
#include "cocos2d/MainScene.h"
#include "ui/UIHelper.h"
#include "ui/UIText.h"
#include "ui/UIButton.h"
#include "ui/UIListView.h"
#include "Platform.h"
#include "cocostudio/ActionTimeline/CCActionTimeline.h"
#include "extensions/GUI/CCScrollView/CCTableView.h"
#include <fmt/format.h>

using namespace cocos2d;
using namespace cocos2d::ui;

NodeMap::NodeMap() : FileName(nullptr) {}

NodeMap::NodeMap(const char *filename, cocos2d::Node *node) :
    FileName(filename) {
    initFromNode(node);
}

template <>
cocos2d::Node *NodeMap::findController<cocos2d::Node>(const std::string &name,
                                                      bool notice) const {
    auto it = this->find(name);
    if(it != this->end())
        return it->second;
    if(notice) {
        TVPShowSimpleMessageBox(
            fmt::format("Node {} not exist in {}", name, FileName),
            "Fail to load ui");
    }
    return nullptr;
}

void NodeMap::initFromNode(cocos2d::Node *node) {
    const Vector<Node *> &childlist = node->getChildren();
    for(auto child : childlist) {
        std::string name = child->getName();
        if(!name.empty())
            (*this)[name] = child;
        initFromNode(child);
    }
}

void NodeMap::onLoadError(const std::string &name) const {
    TVPShowSimpleMessageBox(
        fmt::format("Node {} wrong controller type in {}", name, FileName),
        "Fail to load ui");
}

Node *CSBReader::Load(const char *filename) {
    clear();
    FileName = filename;
    Node *ret = CSLoader::createNode(filename, [this](Ref *p) {
        Node *node = dynamic_cast<Node *>(p);
        std::string name = node->getName();
        if(!name.empty())
            operator[](name) = node;
        int nAction = node->getNumberOfRunningActions();
        if(nAction == 1) {
            auto *action = dynamic_cast<cocostudio::timeline::ActionTimeline *>(
                node->getActionByTag(node->getTag()));
            if(action && action->IsAnimationInfoExists("autoplay")) {
                action->play("autoplay", true);
            }
        }
    });
    if(!ret) {
        TVPShowSimpleMessageBox(filename, "Fail to load ui file");
    }
    return ret;
}

iTVPBaseForm::~iTVPBaseForm() {}

void iTVPBaseForm::Show() {}

bool iTVPBaseForm::initFromFile(const char *navibar, const char *body,
                                const char *bottombar, cocos2d::Node *parent) {
    bool ret = cocos2d::Node::init();
    // NaviBar.Title = nullptr;
    NaviBar.Left = nullptr;
    NaviBar.Right = nullptr;
    NaviBar.Root = nullptr;
    CSBReader reader;
    if(navibar) {
        NaviBar.Root = reader.Load(navibar);
        if(!NaviBar.Root) {
            return false;
        }
        // 		NaviBar.Title =
        // static_cast<Button*>(reader.findController("title")); if
        // (NaviBar.Title)
        // { 			NaviBar.Title->setEnabled(false); // normally
        // 		}
        NaviBar.Left =
            dynamic_cast<Button *>(reader.findController("left", false));
        NaviBar.Right =
            dynamic_cast<Widget *>(reader.findController("right", false));
        bindHeaderController(reader);
    }

    BottomBar.Root = nullptr;
    // BottomBar.Panel = nullptr;
    if(bottombar) {
        BottomBar.Root = reader.Load(bottombar);
        if(!BottomBar.Root) {
            return false;
        }
        // BottomBar.Panel =
        // static_cast<ListView*>(reader.findController("panel"));
        bindFooterController(reader);
    }
    // FIXME: 依靠bug运行， 此处应为非法转换, 危险操作！！
    RootNode = reinterpret_cast<Widget *>(reader.Load(body));
    if(!RootNode) {
        return false;
    }
    if(!parent) {
        parent = this;
    }
    parent->addChild(RootNode);
    if(NaviBar.Root)
        parent->addChild(NaviBar.Root);
    if(BottomBar.Root)
        parent->addChild(BottomBar.Root);
    rearrangeLayout();
    bindBodyController(reader);
    return ret;
}

void iTVPBaseForm::rearrangeLayout() {
    float scale = TVPMainScene::GetInstance()->getUIScale();
    Size sceneSize = TVPMainScene::GetInstance()->getUINodeSize();
    setContentSize(sceneSize);
    Size bodySize = RootNode->getParent()->getContentSize();
    if(NaviBar.Root) {
        Size size = NaviBar.Root->getContentSize();
        size.width = bodySize.width / scale;
        NaviBar.Root->setContentSize(size);
        NaviBar.Root->setScale(scale);
        ui::Helper::doLayout(NaviBar.Root);
        size.height *= scale;
        bodySize.height -= size.height;
        NaviBar.Root->setPosition(0, bodySize.height);
    }
    if(BottomBar.Root) {
        Size size = BottomBar.Root->getContentSize();
        size.width = bodySize.width / scale;
        BottomBar.Root->setContentSize(size);
        BottomBar.Root->setScale(scale);
        ui::Helper::doLayout(BottomBar.Root);
        size.height *= scale;
        bodySize.height -= size.height;
        BottomBar.Root->setPosition(Vec2::ZERO);
    }
    if(RootNode) {
        bodySize.height /= scale;
        bodySize.width /= scale;
        RootNode->setContentSize(bodySize);
        RootNode->setScale(scale);
        ui::Helper::doLayout(RootNode);
        if(BottomBar.Root)
            RootNode->setPosition(
                Vec2(0, BottomBar.Root->getContentSize().height * scale));
    }
}

void iTVPBaseForm::onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode,
                                cocos2d::Event *event) {
    if(keyCode == cocos2d::EventKeyboard::KeyCode::KEY_BACK) {
        TVPMainScene::GetInstance()->popUIForm(
            this, TVPMainScene::eLeaveAniLeaveFromLeft);
    }
}

// void iTVPBaseForm::initBottomBar(std::vector<std::pair<std::string,
// std::function<void()> > > args) { 	if (!BottomBar.Panel) return;
// 	BottomBar.Panel->removeAllItems();
// 	CSBReader reader;
// 	for (auto it : args) {
// 		Widget *root =
// static_cast<Widget*>(reader.Load(it.first.c_str())); 		Widget
// *btn = static_cast<Widget*>(reader.findController("button"));
// std::function<void()> func = it.second; 		if (btn)
// btn->addClickEventListener([=](cocos2d::Ref*){ func(); });
// BottomBar.Panel->pushBackCustomItem(root);
// 	}
// }

void iTVPFloatForm::rearrangeLayout() {
    float scale = TVPMainScene::GetInstance()->getUIScale();
    Size sceneSize = TVPMainScene::GetInstance()->getUINodeSize();
    setContentSize(sceneSize);
    Vec2 center = sceneSize / 2;
    sceneSize.height *= 0.75f;
    sceneSize.width *= 0.75f;
    if(RootNode) {
        sceneSize.width /= scale;
        sceneSize.height /= scale;
        RootNode->setContentSize(sceneSize);
        ui::Helper::doLayout(RootNode);
        RootNode->setScale(scale);
        RootNode->setAnchorPoint(Vec2(0.5f, 0.5f));
        RootNode->setPosition(center);
    }
}

void ReloadTableViewAndKeepPos(cocos2d::extension::TableView *pTableView) {
    Vec2 off = pTableView->getContentOffset();
    float origHeight = pTableView->getContentSize().height;
    pTableView->reloadData();
    off.y += origHeight - pTableView->getContentSize().height;
    bool bounceable = pTableView->isBounceable();
    pTableView->setBounceable(false);
    pTableView->setContentOffset(off);
    pTableView->setBounceable(bounceable);
}
