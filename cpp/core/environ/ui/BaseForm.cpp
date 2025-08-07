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
            dynamic_cast<Button *>(reader.findController("right", false));
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
        if (bottomBar == nullptr) {
            footSize = Size(0, 0); // 如果没有底部栏，则高度为0
            naviSize = Size(parentSize.width, parentSize.height * 0.1f);
            bodySize = Size(parentSize.width, parentSize.height * 0.9f);
        }
    }else{
        naviSize = Size(parentSize.width, parentSize.height * 0.1f);
        bodySize = Size(parentSize.width, parentSize.height * 0.8f);
        footSize = Size(parentSize.width, parentSize.height * 0.1f);
    }

    // naviBar
    if (naviBarBuilder) {
        Widget* naviBar = naviBarBuilder(naviSize, scale);
        naviBar->setContentSize(naviSize);
        auto lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::TOP);
        naviBar->setLayoutParameter(lp);

        NaviBar.Root = naviBar;
        NaviBar.Left = dynamic_cast<Button *>(findChildByNameRecursively(NaviBar.Root,"left"));
        NaviBar.Right = dynamic_cast<Button *>(findChildByNameRecursively(NaviBar.Root,"right"));
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

    float naviscale = 0.1f;
    float bodyscale = 0.8f;
    float footerscale = 0.1f;
    if (bottomBarWidget) {
        // 如果有底部栏，则比例为 10% + 80% + 10%
        naviscale = 0.1f;
        bodyscale = 0.8f;
        footerscale = 0.1f;
    } else {
        // 没有底部栏，则比例为 10% + 90%
        naviscale = 0.1f;
        bodyscale = 0.9f;
        footerscale = 0.0f; // 底部栏高度为0
    }

    // 1) naviBar —— 贴顶
    if (naviBarWidget) {
        naviBarWidget->setContentSize(Size(container->getContentSize().width,
                                    container->getContentSize().height * naviscale));
        auto* lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::TOP);
        naviBarWidget->setLayoutParameter(lp);
        container->addChild(naviBarWidget);
        NaviBar.Root = naviBarWidget;
        NaviBar.Left = dynamic_cast<Button *>(findChildByNameRecursively(NaviBar.Root,"left"));
        NaviBar.Right = dynamic_cast<Button *>(findChildByNameRecursively(NaviBar.Root,"right"));
        bindHeaderController(NaviBar.Root);
    }

    // 2) body ——中间填满
    if (bodyWidget) {
        bodyWidget->setContentSize(Size(container->getContentSize().width,
                                    container->getContentSize().height * bodyscale));
        auto* lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::CENTER_VERTICAL);
        bodyWidget->setLayoutParameter(lp);
        container->addChild(bodyWidget);
        RootNode = bodyWidget;
        bindBodyController(bodyWidget);
    }

    // 3) bottomBar，贴底
    if (bottomBarWidget) {
        bottomBarWidget->setContentSize(Size(container->getContentSize().width,
                                        container->getContentSize().height * footerscale));
        auto* lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::BOTTOM);
        bottomBarWidget->setLayoutParameter(lp);
        container->addChild(bottomBarWidget);
        BottomBar.Root = bottomBarWidget;
        bindFooterController(bottomBarWidget);
    }

    return true;
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
