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
    ret->initFromFile(Csd::createNaviBar(), Csd::createListView(), nullptr);
    PrefListSize = ret->PrefList->getContentSize();
    ret->initPref(config);
    ret->setOnExitCallback([capture0 = GlobalConfigManager::GetInstance()] {
        capture0->SaveToFile();
    });
    return ret;
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
