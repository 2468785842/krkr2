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

iTVPBaseForm::~iTVPBaseForm() = default;

void iTVPBaseForm::Show() {}

/**
 * 递归查找指定名称的子节点
 * @param parent 从这个节点开始向下找
 * @param name   要查找的名字
 * @return       找到的第一个同名节点，找不到返回 nullptr
 */
Widget* findChildByNameRecursively(const Widget* parent, const std::string& name)
{
    if (!parent) return nullptr;

    // 先查直接子节点
    Widget* child = parent->getChildByName<Widget*>(name);
    if (child) return child;

    // 再递归查所有子节点的子节点
    const Vector<Node*>& children = parent->getChildren();
    for (Node* node : children)
    {
        // 只有 Widget 才继续递归
        Widget* widget = dynamic_cast<Widget*>(node);
        if (!widget) continue;

        Widget* result = findChildByNameRecursively(widget, name);
        if (result) return result;
    }
    return nullptr;
}

/**
 * 递归查找指定名称的后代节点（支持 Node 及其所有子类）
 * @param parent 从这个节点开始向下找
 * @param name   要查找的名字
 * @return       找到的第一个同名节点，找不到返回 nullptr
 */
Node* findChildByNameRecursively(const Node* parent, const std::string& name)
{
    if (!parent) return nullptr;

    // 1. 先查直接子节点
    Node* child = parent->getChildByName(name);
    if (child) return child;

    // 2. 再递归查所有子节点的子节点
    for (Node* node : parent->getChildren())
    {
        Node* result = findChildByNameRecursively(node, name);
        if (result) return result;
    }
    return nullptr;
}


bool iTVPBaseForm::initFromBuilder(const Csd::NodeBuilderFn &naviBarCall,
                                const Csd::NodeBuilderFn &bodyCall,
                                const Csd::NodeBuilderFn &bottomBarCall,
                                Node *parent) {

    const bool ret = Node::init();
    const auto scale = TVPMainScene::GetInstance()->getUIScale();

    auto *naviBar = naviBarCall(rearrangeHeaderSize(parent), scale);
    auto *body = bodyCall(rearrangeBodySize(parent), scale);
    auto *bottomBar = bottomBarCall(rearrangeFooterSize(parent), scale);

    RootNode = body;
    if(!RootNode) {
        return false;
    }

    if(!parent) {
        parent = this;
    }

    LinearLayoutParameter *param = nullptr;

    if(naviBar) {
        NaviBar.Root = naviBar->getChildByName("background");
        NaviBar.Left = NaviBar.Root->getChildByName<Button *>("left");
        NaviBar.Right = NaviBar.Root->getChildByName<Button *>("right");
        bindHeaderController(NaviBar.Root);

        param = LinearLayoutParameter::create();
        param->setGravity(LinearLayoutParameter::LinearGravity::TOP);
        naviBar->setLayoutParameter(param);
        parent->addChild(naviBar);
    }

    if(bottomBar) {
        BottomBar.Root = bottomBar;
        bindFooterController(bottomBar);

        param = LinearLayoutParameter::create();
        param->setGravity(LinearLayoutParameter::LinearGravity::BOTTOM);
        bottomBar->setLayoutParameter(param);
        parent->addChild(BottomBar.Root);
    }

    param = LinearLayoutParameter::create();
    param->setGravity(LinearLayoutParameter::LinearGravity::CENTER_VERTICAL);
    body->setLayoutParameter(param);
    parent->addChild(RootNode);

    bindBodyController(RootNode);
    return ret;
}
bool iTVPBaseForm::initFromWidget(Widget* naviBarCall,
                    Widget* bodyCall,
                    Widget* bottomBarCall,
                    Node* parent /* = nullptr */)
{
    if (!Node::init()) return false;

    if (!parent) parent = this;

    // 统一容器
    auto container = Layout::create();
    container->setContentSize(Size(720, 720));   // 你的设计分辨率
    container->setAnchorPoint(Vec2::ZERO);
    container->setPosition(Vec2::ZERO);
    parent->addChild(container);

    // naviBar
    if (naviBarCall) {
        naviBarCall->setAnchorPoint(Vec2::ZERO);
        naviBarCall->setPosition(Vec2(0, 600));  // 顶部
        container->addChild(naviBarCall);
        NaviBar.Root = naviBarCall;
        NaviBar.Left = naviBarCall->getChildByName<Button*>("left");
        NaviBar.Right = naviBarCall->getChildByName<Button*>("right");
    }

    // body
    if (bodyCall) {
        bodyCall->setAnchorPoint(Vec2::ZERO);
        bodyCall->setPosition(Vec2(0, 120));     // naviBar 下方
        container->addChild(bodyCall);
        RootNode = bodyCall;
        bindBodyController(bodyCall);
    }

    // bottomBar
    if (bottomBarCall) {
        bottomBarCall->setAnchorPoint(Vec2::ZERO);
        bottomBarCall->setPosition(Vec2(0, 0));  // 底部
        container->addChild(bottomBarCall);
        BottomBar.Root = bottomBarCall;
        bindFooterController(bottomBarCall);
    }

    return true;
}
void iTVPBaseForm::rearrangeLayout() {
    
}

void iTVPBaseForm::onKeyPressed(cocos2d::EventKeyboard::KeyCode keyCode,
                                cocos2d::Event *event) {
    if(keyCode == cocos2d::EventKeyboard::KeyCode::KEY_BACK) {
        TVPMainScene::GetInstance()->popUIForm(
            this, TVPMainScene::eLeaveAniLeaveFromLeft);
    }
}

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
