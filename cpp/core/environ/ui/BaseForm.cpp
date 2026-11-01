#include "BaseForm.h"
#include "cocos2d.h"
#include "Application.h"
#include "ui/UIWidget.h"
#include "cocos2d/MainScene.h"
#include "ui/UIHelper.h"
#include "ui/UIText.h"
#include "ui/UIButton.h"
#include "ui/UIListView.h"
#include "ui/UILayout.h"
#include "Platform.h"
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

iTVPBaseForm::~iTVPBaseForm() = default;

void iTVPBaseForm::Show() {}

bool iTVPBaseForm::initUILayout(const Csd::NodeBuilderFn &naviBarCall,
                                const Csd::NodeBuilderFn &bodyCall,
                                const Csd::NodeBuilderFn &bottomBarCall,
                                Node *layoutParent) {

    const bool ret = Node::init();
    if(!bodyCall) {
        return false;
    }

    if(!layoutParent) {
        layoutParent = this;
    }

    cocos2d::Size containerSize = layoutParent->getContentSize();
    if(containerSize.equals(cocos2d::Size::ZERO) && layoutParent == this) {
        containerSize = TVPMainScene::GetInstance()->getUINodeSize();
        setContentSize(containerSize);
    }

    Node *attachParent = layoutParent;
    cocos2d::ui::Layout *layoutShell = nullptr;
    if(layoutParent == this) {
        layoutShell = cocos2d::ui::Layout::create();
        layoutShell->setLayoutType(cocos2d::ui::Layout::Type::VERTICAL);
        layoutShell->setContentSize(containerSize);
        layoutShell->setAnchorPoint(cocos2d::Vec2::ZERO);
        layoutShell->setPosition(cocos2d::Vec2::ZERO);
        addChild(layoutShell);
        attachParent = layoutShell;
    }

    const auto scale = TVPMainScene::GetInstance()->getUIScale();
    const bool hasHeader = static_cast<bool>(naviBarCall);
    const bool hasFooter = static_cast<bool>(bottomBarCall);
    const cocos2d::Size bodySize = (hasHeader || hasFooter)
        ? rearrangeBodySize(containerSize)
        : containerSize;

    auto *naviBar = naviBarCall
        ? naviBarCall(rearrangeHeaderSize(containerSize), scale)
        : nullptr;
    auto *body = bodyCall(bodySize, scale);
    auto *bottomBar = bottomBarCall
        ? bottomBarCall(rearrangeFooterSize(containerSize), scale)
        : nullptr;

    RootNode = body;
    if(!RootNode) {
        return false;
    }

    LinearLayoutParameter *param = nullptr;

    if(naviBar) {
        auto *background = naviBar->getChildByName("background");
        NaviBar.Root = background ? background : naviBar;
        if(background) {
            NaviBar.Left = NaviBar.Root->getChildByName<Button *>("left");
            NaviBar.Right = NaviBar.Root->getChildByName<Button *>("right");
        } else {
            NaviBar.Left = nullptr;
            NaviBar.Right = nullptr;
        }
        bindHeaderController(NaviBar.Root);

        param = LinearLayoutParameter::create();
        param->setGravity(LinearLayoutParameter::LinearGravity::TOP);
        naviBar->setLayoutParameter(param);
        attachParent->addChild(naviBar);
    }

    if(bottomBar) {
        BottomBar.Root = bottomBar;
        bindFooterController(bottomBar);

        param = LinearLayoutParameter::create();
        param->setGravity(LinearLayoutParameter::LinearGravity::BOTTOM);
        bottomBar->setLayoutParameter(param);
        attachParent->addChild(BottomBar.Root);
    }

    param = LinearLayoutParameter::create();
    param->setGravity(LinearLayoutParameter::LinearGravity::CENTER_VERTICAL);
    body->setLayoutParameter(param);
    attachParent->addChild(RootNode);

    bindBodyController(RootNode);
    if(layoutShell) {
        ui::Helper::doLayout(layoutShell);
    }
    return ret;
}

void iTVPBaseForm::rearrangeLayout() {}

void iTVPBaseForm::onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode,
                                cocos2d::Event *event) {
    if(keyCode == cocos2d::EventKeyboard::KeyCode::KEY_BACK) {
        TVPMainScene::GetInstance()->popUIForm(
            this, TVPMainScene::eLeaveAniLeaveFromLeft);
    }
}

void iTVPFloatForm::rearrangeLayout() {
    float scale = TVPMainScene::GetInstance()->getUIScale();
    cocos2d::Size sceneSize = TVPMainScene::GetInstance()->getUINodeSize();
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
