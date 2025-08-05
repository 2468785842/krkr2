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


bool iTVPBaseForm::initFromBuilder(const Csd::NodeBuilderFn& naviBarBuilder,
                     const Csd::NodeBuilderFn& bodyBuilder,
                     const Csd::NodeBuilderFn& bottomBarBuilder,
                     Node* parent /* = nullptr */)
{
    if (!Node::init()) return false;
    if (!parent) parent = this;

    /* 1. 统一容器：纵向线性布局，占满父节点 */
    auto container = Layout::create();
    container->setContentSize(parent->getContentSize());
    container->setLayoutType(Layout::Type::VERTICAL);
    container->setAnchorPoint(Vec2::ZERO);
    parent->addChild(container);

    /* 2. 计算三栏尺寸（比例 + 缩放） */
    const float scale = TVPMainScene::GetInstance()->getUIScale();
    const Size  parentSize = parent->getContentSize();

    Size naviSize  = rearrangeHeaderSize(parent);   // 10 %
    Size bodySize  = rearrangeBodySize(parent);     // 80 %
    Size footSize  = rearrangeFooterSize(parent);   // 10 %

    /* 3. 创建并添加三栏，用 LinearLayoutParameter 自动排布 */
    if(bottomBarBuilder){
        Widget* bottomBar = bottomBarBuilder(footSize, scale);
        if (bottomBar != nullptr) {
            footSize = Size(0, 0); // 如果没有底部栏，则高度为0
            naviSize = Size(parentSize.width, parentSize.height * 0.1f);
            bodySize = Size(parentSize.width, parentSize.height * 0.9f);
        }
    }else{
        naviSize = Size(parentSize.width, parentSize.height * 0.1f);
        bodySize = Size(parentSize.width, parentSize.height * 0.9f);
    }

    // naviBar
    if (naviBarBuilder) {
        Widget* naviBar = naviBarBuilder(naviSize, scale);
        naviBar->setContentSize(naviSize);
        auto lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::TOP);
        naviBar->setLayoutParameter(lp);

        NaviBar.Root = naviBar->getChildByName<Node*>("background");
        NaviBar.Left = NaviBar.Root->getChildByName<Button*>("left");
        NaviBar.Right = NaviBar.Root->getChildByName<Button*>("right");
        bindHeaderController(NaviBar.Root);

        container->addChild(naviBar);
    }

    // body
    if (bodyBuilder) {
        Widget* body = bodyBuilder(bodySize, scale);
        body->setContentSize(bodySize);
        auto lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::CENTER_VERTICAL);
        body->setLayoutParameter(lp);

        RootNode = body;
        bindBodyController(RootNode);
        container->addChild(RootNode);
    }

    // bottomBar
    if (bottomBarBuilder) {
        Widget* bottomBar = bottomBarBuilder(footSize, scale);
        if (bottomBar != nullptr) {
            bottomBar->setContentSize(footSize);
            auto lp = LinearLayoutParameter::create();
            lp->setGravity(LinearLayoutParameter::LinearGravity::BOTTOM);
            bottomBar->setLayoutParameter(lp);
            BottomBar.Root = bottomBar;
            bindFooterController(bottomBar);
            container->addChild(bottomBar);
        }
    }

    return true;
}
bool iTVPBaseForm::initFromWidget(Widget* naviBarWidget,
                    Widget* bodyWidget,
                    Widget* bottomBarWidget,
                    Node* parent /* = nullptr */)
{
    if (!Node::init()) return false;

    if (!parent) parent = this;

    // 统一容器
    auto container = Layout::create();
    container->setContentSize(parent->getContentSize());
    container->setLayoutType(Layout::Type::VERTICAL);
    container->setAnchorPoint(Vec2::ZERO);
    parent->addChild(container);

    // 1) naviBar —— 占 10% 高度，贴顶
    if (naviBarWidget) {
        naviBarWidget->setContentSize(Size(container->getContentSize().width,
                                    container->getContentSize().height * 0.10f));
        auto* lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::TOP);
        naviBarWidget->setLayoutParameter(lp);
        container->addChild(naviBarWidget);
    }

    // 2) body —— 占 80% 高度，中间填满
    if (bodyWidget) {
        bodyWidget->setContentSize(Size(container->getContentSize().width,
                                    container->getContentSize().height * 0.80f));
        auto* lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::CENTER_VERTICAL);
        bodyWidget->setLayoutParameter(lp);
        container->addChild(bodyWidget);
        RootNode = bodyWidget;
        bindBodyController(bodyWidget);
    }

    // 3) bottomBar —— 占 10% 高度，贴底
    if (bottomBarWidget) {
        bottomBarWidget->setContentSize(Size(container->getContentSize().width,
                                        container->getContentSize().height * 0.10f));
        auto* lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::BOTTOM);
        bottomBarWidget->setLayoutParameter(lp);
        container->addChild(bottomBarWidget);
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
