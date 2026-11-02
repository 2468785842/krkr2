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
    auto *ret = new TVPGlobalPreferenceForm();
    ret->autorelease();
    ret->initUILayout(Csd::createNaviBar, Csd::createListView,
                      Csd::NodeBuilderFn{});
    PrefListSize = ret->PrefList->getContentSize();
    ret->initPref(config);
    ret->setOnExitCallback([capture0 = GlobalConfigManager::GetInstance()] {
        capture0->SaveToFile();
    });
    return ret;
}

void TVPGlobalPreferenceForm::Initialize() {
    TVPInitializeRuntimePreferences();
    initAllConfig();
}
