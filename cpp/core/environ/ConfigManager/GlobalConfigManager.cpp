#include "GlobalConfigManager.h"
#include "ConfigXml.h"
#include "PreferenceStorage.h"

#include <locale>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <utility>

namespace {
    auto &launchOverrides() {
        static std::map<std::string, std::string> overrides;
        return overrides;
    }

    const std::string *findLaunchOverride(const std::string &name) {
        const auto &overrides = launchOverrides();
        const auto it = overrides.find(name);
        return it == overrides.end() ? nullptr : &it->second;
    }
} // namespace

void iSysConfigManager::SetLaunchOverrides(
    std::map<std::string, std::string> overrides) {
    launchOverrides() = std::move(overrides);
}

GlobalConfigManager::GlobalConfigManager() { Initialize(); }

GlobalConfigManager *GlobalConfigManager::GetInstance() {
    static GlobalConfigManager instance;
    return &instance;
}

void iSysConfigManager::Initialize() {
    AllConfig.clear();
    ConfigUpdated = false;

    std::string xml;
    if(!krkr::config::RequirePreferenceStorage()->Read(GetFilePath(), xml))
        return;
    const auto preferences = krkr::config::ReadPreferences(xml);
    if(!preferences)
        return;
    AllConfig = preferences->values;
    CustomArguments.insert(CustomArguments.end(),
                           preferences->arguments.begin(),
                           preferences->arguments.end());
    KeyMap.insert(preferences->keyMap.begin(), preferences->keyMap.end());
}

void iSysConfigManager::SaveToFile() {
    if(!ConfigUpdated)
        return;
    std::string filepath = GetFilePath();
    if(filepath.empty())
        return;
    const auto xml =
        krkr::config::WritePreferences({ AllConfig, CustomArguments, KeyMap });
    const auto storage = krkr::config::RequirePreferenceStorage();
    if(!storage->Write(filepath, xml))
        storage->ReportWriteFailure();
    ConfigUpdated = false;
}

bool iSysConfigManager::IsValueExist(const std::string &name) {
    auto it = AllConfig.find(name);
    return it != AllConfig.end();
}

std::string GlobalConfigManager::GetFilePath() {
    return TVPGetInternalPreferencePath() + "GlobalPreference.xml";
}

template <>
bool iSysConfigManager::GetValue<bool>(const std::string &name,
                                       const bool &defVal) {
    return !!GetValue<int>(name, defVal);
}

template <>
int iSysConfigManager::GetValue<int>(const std::string &name,
                                     const int &defVal) {
    if(const auto *override = findLaunchOverride(name))
        return atoi(override->c_str());
    auto it = AllConfig.find(name);
    if(it == AllConfig.end()) {
        SetValueInt(name, defVal);
        return defVal;
    }
    return atoi(it->second.c_str());
}

template <>
float iSysConfigManager::GetValue<float>(const std::string &name,
                                         const float &defVal) {
    if(const auto *override = findLaunchOverride(name)) {
        std::istringstream input(*override);
        input.imbue(std::locale::classic());
        float value = defVal;
        input >> value;
        return value;
    }
    auto it = AllConfig.find(name);
    if(it == AllConfig.end()) {
        SetValueFloat(name, defVal);
        return defVal;
    }
    return atof(it->second.c_str());
}

template <>
std::string
iSysConfigManager::GetValue<std::string>(const std::string &name,
                                         const std::string &defVal) {
    if(const auto *override = findLaunchOverride(name))
        return *override;
    auto it = AllConfig.find(name);
    if(it == AllConfig.end()) {
        SetValue(name, defVal);
        return defVal;
    }
    return it->second;
}

void iSysConfigManager::SetValueInt(const std::string &name, int val) {
    char buf[16];
    sprintf(buf, "%d", val);
    AllConfig[name] = buf;
    ConfigUpdated = true;
}

void iSysConfigManager::SetValueFloat(const std::string &name, float val) {
    char buf[24];
    sprintf(buf, "%g", val);
    AllConfig[name] = buf;
    ConfigUpdated = true;
}

void iSysConfigManager::SetValue(const std::string &name,
                                 const std::string &val) {
    AllConfig[name] = val;
    ConfigUpdated = true;
}

void iSysConfigManager::SetKeyMap(int k /* 0 means remove */, int v) {
    if(v == 0) {
        KeyMap.erase(k);
    } else {
        KeyMap[k] = v;
    }
}

std::vector<std::string> iSysConfigManager::GetCustomArgumentsForPush() {
    std::vector<std::string> ret;
    for(auto arg : CustomArguments) {
        std::string line("-");
        line += arg.first;
        line += "=";
        line += arg.second;
        ret.emplace_back(line);
    }
    return ret;
}
