#include "IndividualPreferenceForm.h"
#include "ConfigManager/LocaleConfigManager.h"
#include "ui/UIButton.h"
#include "cocos2d/MainScene.h"
#include "ui/UIListView.h"
#include "platform/CCFileUtils.h"
#include "ConfigManager/IndividualConfigManager.h"
#include "Platform.h"
#include "csd/CsdUIFactory.h"

using namespace cocos2d;
using namespace cocos2d::ui;
#define INDIVIDUAL_PREFERENCE

#define TVPGlobalPreferenceForm IndividualPreferenceForm

static iSysConfigManager *GetConfigManager() {
    return IndividualConfigManager::GetInstance();
}
#include "PreferenceConfig.h"

#undef TVPGlobalPreferenceForm

static void initInividualConfig() {
    if(!RootPreference.Preferences.empty())
        return;
    initAllConfig();
    RootPreference.Title = "preference_title_individual";
}

IndividualPreferenceForm *
IndividualPreferenceForm::create(const tPreferenceScreen *config) {
    initInividualConfig();
    if(!config)
        config = &RootPreference;
    auto *ret = new IndividualPreferenceForm();
    ret->autorelease();
    ret->initFromFile(Csd::createNaviBar(), Csd::createListView(), nullptr);
    PrefListSize = ret->PrefList->getContentSize();
    ret->initPref(config);
    ret->setOnExitCallback([capture0 = IndividualConfigManager::GetInstance()] {
        capture0->SaveToFile();
    });
    return ret;
}
