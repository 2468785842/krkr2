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
    IndividualPreferenceForm *ret = new IndividualPreferenceForm();
    ret->autorelease();
    ret->initFromBuilder(Csd::createNaviBar, Csd::createListView, nullptr,nullptr);
    PrefListSize = ret->PrefList->getContentSize();
    ret->initPref(config);
    ret->setOnExitCallback(std::bind(&IndividualConfigManager::SaveToFile,
                                     IndividualConfigManager::GetInstance()));
    return ret;
}
