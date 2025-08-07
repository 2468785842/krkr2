#include "AppDelegate.h"
#include "MainScene.h"
#include "environ/Application.h"
#include "environ/Platform.h"
#include "environ/ui/MessageBox.h"
#include "environ/ui/GlobalPreferenceForm.h"
#include "environ/ui/MainFileSelectorForm.h"
#include "environ/ui/extension/UIExtension.h"
#include "environ/ConfigManager/LocaleConfigManager.h"
#include "environ/ConfigManager/GlobalConfigManager.h"

extern "C" void SDL_SetMainReady();
extern std::thread::id TVPMainThreadID;

static cocos2d::Size designResolutionSize(1920, 1080);
const  cocos2d::Size screenSize(1280, 720);

bool TVPCheckStartupArg();

std::string TVPGetCurrentLanguage();

void TVPAppDelegate::applicationWillEnterForeground() {
    ::Application->OnActivate();
    cocos2d::Director::getInstance()->startAnimation();
}

void TVPAppDelegate::applicationDidEnterBackground() {
    ::Application->OnDeactivate();
    cocos2d::Director::getInstance()->stopAnimation();
}

bool TVPAppDelegate::applicationDidFinishLaunching() {
    SDL_SetMainReady();
    TVPMainThreadID = std::this_thread::get_id();
    cocos2d::log("applicationDidFinishLaunching");
    // initialize director
    auto director = cocos2d::Director::getInstance();
    auto glview = director->getOpenGLView();
    if(!glview) {
        glview = cocos2d::GLViewImpl::create("kirikiri2");
        director->setOpenGLView(glview);
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32 
    // 1. 设置物理窗口大小（实际窗口大小）
    glview->setFrameSize(screenSize.width, screenSize.height);

    // 3. 获取 Win32 窗口句柄
    HWND hwnd = glview->getWin32Window();
    if (hwnd) {
        // 添加可调节边框和最大化按钮
        LONG style = GetWindowLong(hwnd, GWL_STYLE);
        style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
        SetWindowLong(hwnd, GWL_STYLE, style);

    }
#endif
    }
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
    // Set the design resolution
    glview->setDesignResolutionSize(designResolutionSize.width,designResolutionSize.height,
                                    ResolutionPolicy::EXACT_FIT);
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID)
     // Set the design resolution
    cocos2d::Size screenSize = glview->getFrameSize();
    if(screenSize.width < screenSize.height) {
        std::swap(screenSize.width, screenSize.height);
    }
    cocos2d::Size designSize = designResolutionSize;
    designSize.height = designSize.width * screenSize.height / screenSize.width;
    glview->setDesignResolutionSize(screenSize.width, screenSize.height,
                                    ResolutionPolicy::EXACT_FIT);
#else
    CCLOG("This Plafrom don not support");
#endif
#if _DEBUG
    auto visibleSize = director->getVisibleSize();
    auto frameSize = glview->getFrameSize();
    CCLOG("VisibleSize: %f x %f", visibleSize.width, visibleSize.height);
    CCLOG("FrameSize: %f x %f", frameSize.width, frameSize.height);
#endif
    std::vector<std::string> searchPath;
    // In this demo, we select resource according to the frame's
    // height. If the resource size is different from design
    // resolution size, you need to set contentScaleFactor. We use the
    // ratio of resource's height to the height of design resolution,
    // this can make sure that the resource's height could fit for the
    // height of design resolution.
    searchPath.emplace_back("res");

    // set searching path
    cocos2d::FileUtils::getInstance()->setSearchPaths(searchPath);

    // turn on display FPS
    director->setDisplayStats(false);

    // set FPS. the default value is 1.0/60 if you don't call this
    director->setAnimationInterval(1.0f / 60);

    TVPInitUIExtension();

    // initialize something
    LocaleConfigManager::GetInstance()->Initialize(TVPGetCurrentLanguage());
    // create a scene. it's an autorelease object
    TVPMainScene *scene = TVPMainScene::CreateInstance();

    // run
    director->runWithScene(scene);

    scene->scheduleOnce(
        [](float dt) {
            TVPMainScene::GetInstance()->unschedule("launch");
            TVPGlobalPreferenceForm::Initialize();
            if(!TVPCheckStartupArg()) {
                TVPMainScene::GetInstance()->pushUIForm(
                    TVPMainFileSelectorForm::create());
            }
        },
        0, "launch");
// 让copcos2d自动计算
#if 0
    float scale = std::min(
        static_cast<float>(screenSize.width)  / designResolutionSize.width,
        static_cast<float>(screenSize.height) / designResolutionSize.height
    );
#endif

    return true;
}

void TVPAppDelegate::initGLContextAttrs() {
    GLContextAttrs glContextAttrs = { 8, 8, 8, 8, 24, 8 };
    cocos2d::GLView::setGLContextAttrs(glContextAttrs);
}


void TVPOpenPatchLibUrl() {
    cocos2d::Application::getInstance()->openURL(
        "https://zeas2.github.io/Kirikiroid2_patch/patch");
}
