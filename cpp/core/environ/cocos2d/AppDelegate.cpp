#include "TVPLog.h"
#include "AppDelegate.h"

#include "MainScene.h"
#include "HostLifecycle.h"
#include "Platform.h"
#if KRKR2_USE_COCOS_UI
#include "ui/MainFileSelectorForm.h"
#endif
#include "ConfigManager/PreferenceDefaults.h"
#include "ConfigManager/LocaleConfigManager.h"

#include "audio/include/AudioEngine.h"

#include "EnvironFileBackendInit.h"
#include "EngineArchiveInit.h"
#include "DialogBackend.h"
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) ||                               \
    (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) ||                                 \
    (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
#include "launch/DesktopLaunch.h"
#endif

static cocos2d::Size designResolutionSize(960, 640);

extern std::thread::id TVPMainThreadID;

bool TVPCheckStartupArg();

std::string TVPGetCurrentLanguage();

void TVPAppDelegate::applicationWillEnterForeground() {
    TVPOnHostForeground();
    cocos2d::Director::getInstance()->startAnimation();
}

void TVPAppDelegate::applicationDidEnterBackground() {
    TVPOnHostBackground();
    cocos2d::Director::getInstance()->stopAnimation();
}

bool TVPAppDelegate::applicationDidFinishLaunching() {
    TVPMainThreadID = std::this_thread::get_id();
    TVPInitNativeDialogBackend();
    G_CoreLog.debug("App Finish Launching");

    // 文件选择 / 命令行启动会在 StartApplication 之前调用 TVPCheckArchive
    TVPInitEnvironFileBackends();
    TVPInitEngineArchiveCreators();

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
        glview = cocos2d::GLViewImpl::create("krkr2");
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
    // glview->setFrameSize(designResolutionSize.width * 1.5f,
    //                      designResolutionSize.height * 1.5f);
#endif

    std::vector<std::string> searchPath;

    searchPath.emplace_back("res");

    // set searching path
    cocos2d::FileUtils::getInstance()->setSearchPaths(searchPath);

    // turn on display FPS
    director->setDisplayStats(false);

    // set FPS. the default value is 1.0/60 if you don't call this
    director->setAnimationInterval(1.0f / 60);

    // initialize something
    LocaleConfigManager::GetInstance()->Initialize(TVPGetCurrentLanguage());
    // create a scene. it's an autorelease object
    TVPMainScene *scene = TVPMainScene::CreateInstance();

    // run
    director->runWithScene(scene);

    scene->scheduleOnce(
        [](float dt) {
            TVPMainScene::GetInstance()->unschedule("launch");
            TVPInitializeRuntimePreferences();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) ||                               \
    (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) ||                                 \
    (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
            // Retain platform startup services (for example optional dump
            // upload).
            TVPCheckStartupArg();
            const auto &target = krkr::launch::DesktopTarget();
            if(!target.empty()) {
                G_CoreLog.info("Direct game startup: {}", target);
                TVPMainScene::GetInstance()->startupFrom(target);
            } else {
#else
            if(!TVPCheckStartupArg()) {
#endif
#if KRKR2_USE_COCOS_UI
                TVPMainScene::GetInstance()->pushUIForm(
                    TVPMainFileSelectorForm::create());
#endif
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
