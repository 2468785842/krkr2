#include "GlobalConfigManager.h"
#include "ConfigFormat.h"
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
    CustomArguments.clear();
    KeyMap.clear();
    ConfigUpdated = false;

    const auto storage = krkr::config::RequirePreferenceStorage();
    const auto path = GetFilePath();
    std::string data;
    std::optional<krkr::config::Preferences> preferences;
    bool migrated = false;
    if(storage->Exists(path)) {
        if(!storage->Read(path, data))
            return;
        preferences = krkr::config::ReadPreferences(data);
    } else if(path.size() >= 5 && path.substr(path.size() - 5) == ".json") {
        if(!storage->Read(path.substr(0, path.size() - 5) + ".xml", data))
            return;
        preferences = krkr::config::ReadLegacyPreferences(data);
        migrated = true;
    }
    if(!preferences)
        return;
    AllConfig = preferences->values;
    CustomArguments = preferences->arguments;
    KeyMap = preferences->keyMap;
    if(migrated) {
        ConfigUpdated = true;
        SaveToFile();
    }
}

void iSysConfigManager::SaveToFile() {
    if(!ConfigUpdated)
        return;
    std::string filepath = GetFilePath();
    if(filepath.empty())
        return;
    const auto json =
        krkr::config::WritePreferences({ AllConfig, CustomArguments, KeyMap });
    const auto storage = krkr::config::RequirePreferenceStorage();
    if(!storage->Write(filepath, json))
        storage->ReportWriteFailure();
    ConfigUpdated = false;
}

bool iSysConfigManager::IsValueExist(const std::string &name) {
    auto it = AllConfig.find(name);
    return it != AllConfig.end();
}

std::string GlobalConfigManager::GetFilePath() {
    return TVPGetInternalPreferencePath() + "GlobalPreference.json";
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
