#include <spdlog/spdlog.h>
#include "AppDelegate.h"

#include "MainScene.h"
#include "Application.h"
#include "Platform.h"
#include "ui/GlobalPreferenceForm.h"
#include "ui/MainFileSelectorForm.h"
#include "ui/extension/UIExtension.h"
#include "ConfigManager/LocaleConfigManager.h"

#include "audio/include/AudioEngine.h"

static cocos2d::Size designResolutionSize(960, 640);

extern std::thread::id TVPMainThreadID;

extern "C" void SDL_SetMainReady();

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
    spdlog::debug("App Finish Launching");
    // initialize director
    auto director = cocos2d::Director::getInstance();
    auto glview = director->getOpenGLView();
    if(!glview) {
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) ||                               \
    (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) ||                                 \
    (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
        glview = cocos2d::GLViewImpl::createWithRect(
            "krkr2",
            cocos2d::Rect(0, 0, designResolutionSize.width,
                          designResolutionSize.height));
#else
        glview = GLViewImpl::create("krkr2");
#endif
        director->setOpenGLView(glview);
#if CC_TARGET_PLATFORM == CC_PLATFORM_WIN32
        if(HWND hwnd = glview->getWin32Window()) {
            // 添加可调节边框和最大化按钮
            LONG style = GetWindowLong(hwnd, GWL_STYLE);
            style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
            SetWindowLong(hwnd, GWL_STYLE, style);
        }
#endif
    }

#if (CC_TARGET_PLATFORM == CC_PLATFORM_ANDROID ||                              \
     CC_TARGET_PLATFORM == CC_PLATFORM_IOS)
    // Set the design resolution
    cocos2d::Size screenSize = glview->getFrameSize();
    if(screenSize.width < screenSize.height) {
        std::swap(screenSize.width, screenSize.height);
    }
    cocos2d::Size ds = designResolutionSize;
    ds.height = ds.width * screenSize.height / screenSize.width;
    glview->setDesignResolutionSize(screenSize.width, screenSize.height,
                                    ResolutionPolicy::EXACT_FIT);
#else
    // Set the design resolution
    glview->setDesignResolutionSize(designResolutionSize.width,
                                    designResolutionSize.height,
                                    ResolutionPolicy::SHOW_ALL);
    glview->setFrameSize(designResolutionSize.width * 1.5f,
                         designResolutionSize.height * 1.5f);
#endif

    std::vector<std::string> searchPath;

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
