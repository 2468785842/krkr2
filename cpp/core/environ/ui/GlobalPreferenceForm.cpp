#include "GlobalPreferenceForm.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "ui/UIButton.h"
#include "cocos2d/MainScene.h"
#include "ui/UIListView.h"
#include "ConfigManager/GlobalConfigManager.h"
#include "platform/CCFileUtils.h"
#include "Platform.h"
#include "csd/CsdUIFactory.h"

using namespace cocos2d;
using namespace cocos2d::ui;
#define GLOBAL_PREFERENCE

static iSysConfigManager *GetConfigManager() {
    return GlobalConfigManager::GetInstance();
}
#include "PreferenceConfig.h"

TVPGlobalPreferenceForm *
TVPGlobalPreferenceForm::create(const tPreferenceScreen *config) {
    Initialize();
    if(!config)
        config = &RootPreference;
    TVPGlobalPreferenceForm *ret = new TVPGlobalPreferenceForm();
    ret->autorelease();
    ret->initFromWidget(createNaviBar(), createListView(),
                         nullptr, nullptr);
    PrefListSize = ret->PrefList->getContentSize();
    ret->initPref(config);
    ret->setOnExitCallback(std::bind(&GlobalConfigManager::SaveToFile,
                                     GlobalConfigManager::GetInstance()));
    return ret;
}

bool TVPGlobalPreferenceForm::initFromBuilder(
    const Csd::NodeBuilderFn &naviBarBuilder,
    const Csd::NodeBuilderFn &bodyBuilder,
    const Csd::NodeBuilderFn &bottomBarBuilder, Node *parent) {

    auto ret = Node::init();
    if(!ret) return false;
    if(!parent) parent = this;
    Size size = TVPMainScene::GetInstance()->getContentSize();
    float scale = parent->getScale();
    auto container = Layout::create();
    container->setContentSize(size);
    container->setLayoutType(Layout::Type::VERTICAL);
    spdlog::info("container size: {}, {} location <{}, {}>", container->getContentSize().width, container->getContentSize().height, container->getPosition().x, container->getPosition().y);
    container->setName("container");
    parent->addChild(container);
    // 导航栏
    if(naviBarBuilder) {
        Widget *naviBar = naviBarBuilder(Size(size.width,size.height*0.2f), scale);
        #if _DEBUG
        spdlog::info("naviBar size: {}, {} location <{}, {}>", naviBar->getContentSize().width, naviBar->getContentSize().height, naviBar->getPosition().x, naviBar->getPosition().y);
        #endif
        auto lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::TOP);
        naviBar->setLayoutParameter(lp);

        NaviBar.Root = naviBar;
        NaviBar.Left = NaviBar.Root->getChildByName<Button *>("left");
        NaviBar.Right = NaviBar.Root->getChildByName<Button *>("right");
        bindHeaderController(NaviBar.Root);
        container->addChild(naviBar);
    }
    // 主体内容
    if(bodyBuilder) {
        Widget *body = bodyBuilder(Size(size.width, size.height * 0.8f), scale);
        auto lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::TOP);
        body->setLayoutParameter(lp);
        RootNode = body;
        bindBodyController(RootNode);
        container->addChild(RootNode);
    } else {
        // 如果没有主体内容，则创建一个空的 ListView
        RootNode = ListView::create();
        RootNode->setContentSize(rearrangeBodySize(parent));
        container->addChild(RootNode);
    }
    
    PrefList = static_cast<ListView *>(RootNode->getChildByName("list"));
    return true;
   
}

bool TVPGlobalPreferenceForm::initFromWidget(Widget *naviBarWidget,
                                        Widget *bodyWidget,
                                        Widget *bottomBarWidget,
                                        Node *parent /* = nullptr */) {
    auto ret = Node::init();
    if(!ret) return false;
    if(!parent) parent = this;
    Size size = TVPMainScene::GetInstance()->getContentSize();
    float scale = parent->getScale();
    auto container = Layout::create();
    container->setContentSize(size);
    container->setLayoutType(Layout::Type::VERTICAL);
    spdlog::info("container size: {}, {} location <{}, {}>", container->getContentSize().width, container->getContentSize().height, container->getPosition().x, container->getPosition().y);
    container->setName("container");
    parent->addChild(container);
    // 导航栏
    if(naviBarWidget) {
        Widget *naviBar = naviBarWidget;
        #if _DEBUG
        spdlog::info("naviBar size: {}, {} location <{}, {}>", naviBar->getContentSize().width, naviBar->getContentSize().height, naviBar->getPosition().x, naviBar->getPosition().y);
        #endif
        auto lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::TOP);
        naviBar->setLayoutParameter(lp);

        NaviBar.Root = naviBar;
        NaviBar.Left = NaviBar.Root->getChildByName<Button *>("left");
        NaviBar.Right = NaviBar.Root->getChildByName<Button *>("right");
        bindHeaderController(NaviBar.Root);
        container->addChild(naviBar);
    }
    // 主体内容
    if(bodyWidget) {
        Widget *body = bodyWidget;
        auto lp = LinearLayoutParameter::create();
        lp->setGravity(LinearLayoutParameter::LinearGravity::TOP);
        body->setLayoutParameter(lp);
        RootNode = body;
        bindBodyController(RootNode);
        container->addChild(RootNode);
    } else {
        // 如果没有主体内容，则创建一个空的 ListView
        RootNode = ListView::create();
        RootNode->setContentSize(rearrangeBodySize(parent));
        container->addChild(RootNode);
    }
    
    PrefList = static_cast<ListView *>(RootNode->getChildByName("list"));
    return true;
}
static void WalkConfig(tPreferenceScreen *pref) {
    for(iTVPPreferenceInfo *info : pref->Preferences) {
        info->InitDefaultConfig();
        tPreferenceScreen *subpref = info->GetSubScreenInfo();
        if(subpref) {
            WalkConfig(subpref);
        }
    }
}

void TVPGlobalPreferenceForm::Initialize() {
    static bool Inited = false;
    if(!Inited) {
        Inited = true;
        if(!GlobalConfigManager::GetInstance()->IsValueExist(
               "GL_EXT_shader_framebuffer_fetch")) {
            // disable GL_EXT_shader_framebuffer_fetch normally for
            // adreno GPU
            if(strstr((const char *)glGetString(GL_RENDERER), "Adreno")) {
                GlobalConfigManager::GetInstance()->SetValueInt(
                    "GL_EXT_shader_framebuffer_fetch", 0);
            }
        }

        initAllConfig();
        WalkConfig(&RootPreference);
        WalkConfig(&SoftRendererOptPreference);
        WalkConfig(&OpenglOptPreference);
    }
}

Widget *TVPGlobalPreferenceForm::createNaviBar() {
    Layout *naviBar = Layout::create();
    naviBar->setName("naviBar");
    naviBar->setLayoutType(Layout::Type::HORIZONTAL);
    // 左侧按钮
    Button *left = Button::create("img/back_btn_off.png", "img/back_btn_on.png");
    auto leftParam = LinearLayoutParameter::create();
    leftParam->setGravity(LinearLayoutParameter::LinearGravity::LEFT);
    leftParam->setMargin(Margin(0, 0, 0, 0));
    left->setLayoutParameter(leftParam);
    left->setName("left");
    naviBar->addChild(left);

    // 中间标题
    Button* title = Button::create("img/empty.png", "img/gray.png");
    title->setTitleText("标题");
    naviBar->addChild(title);

    auto titleParam = LinearLayoutParameter::create();
    titleParam->setGravity(LinearLayoutParameter::LinearGravity::CENTER_VERTICAL);
    titleParam->setMargin(Margin(0, 0, 0, 0));
    title->setLayoutParameter(titleParam);
    naviBar->addChild(title);
    
    // 右侧占位
    Layout *right = Layout::create();
    right->setContentSize(Size(100, 100)); // 占位大小
    right->setTouchEnabled(true);
    right->setName("right");
    auto rightParam = LinearLayoutParameter::create();
    rightParam->setGravity(LinearLayoutParameter::LinearGravity::RIGHT);
    rightParam->setMargin(Margin(0, 0, 0, 0));
    right->setLayoutParameter(rightParam);
    naviBar->addChild(right);

    return naviBar;
}

Widget *TVPGlobalPreferenceForm::createBody() {
    auto body = ListView::create();
    body->setName("list");
    body->setContentSize(PrefListSize);
    body->setDirection(ScrollView::Direction::VERTICAL);
    body->setGravity(ListView::Gravity::CENTER_VERTICAL);
    body->setItemsMargin(10.0f);
    body->setBounceEnabled(true);
    body->setScrollBarEnabled(false);
    return body;
}