#include <cocos2d.h>
#include "ConfigManager/GlobalConfigManager.h"
#include "ConfigManager/PreferenceDefaults.h"

#include <cstring>

void TVPInitializeRuntimePreferences() {
    static bool initialized = false;
    if(initialized)
        return;
    initialized = true;
    const auto *renderer =
        reinterpret_cast<const char *>(glGetString(GL_RENDERER));
    TVPApplyRuntimePreferenceDefaults(*GlobalConfigManager::GetInstance(),
                                      renderer &&
                                          std::strstr(renderer, "Adreno"));
}
