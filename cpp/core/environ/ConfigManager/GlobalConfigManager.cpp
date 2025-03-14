#include "GlobalConfigManager.h"
#include "tinyxml2/tinyxml2.h"
#include "Platform.h"
#include "UtilStreams.h"
#include "LocaleConfigManager.h"

bool TVPWriteDataToFile(const ttstr &filepath, const void *data,
                        unsigned int len);
class XMLMemPrinter : public tinyxml2::XMLPrinter {
    tTVPMemoryStream _stream;
    char _buffer[4096];

public:
    virtual void Print(const char *format, ...) {
        va_list param;
        va_start(param, format);
        int n = vsnprintf(_buffer, 4096, format, param);
        va_end(param);
        _stream.Write(_buffer, n);
    }
    void SaveFile(const std::string &path) {
        if(!TVPWriteDataToFile(path, _stream.GetInternalBuffer(),
                               _stream.GetSize())) {
            TVPShowSimpleMessageBox(LocaleConfigManager::GetInstance()->GetText(
                                        "cannot_create_preference"),
                                    LocaleConfigManager::GetInstance()->GetText(
                                        "readonly_storage"));
        }
    }
};

GlobalConfigManager::GlobalConfigManager() { Initialize(); }

GlobalConfigManager *GlobalConfigManager::GetInstance() {
    static GlobalConfigManager instance;
    return &instance;
}

void iSysConfigManager::Initialize() {
    AllConfig.clear();
    ConfigUpdated = false;

    tinyxml2::XMLDocument doc;

    FILE *fp = nullptr;
    fp = fopen(GetFilePath().c_str(), "rb");

    if(fp && !doc.LoadFile(fp)) {
        tinyxml2::XMLElement *rootElement = doc.RootElement();
        if(rootElement) {
            for(tinyxml2::XMLElement *item =
                    rootElement->FirstChildElement("Item");
                item; item = item->NextSiblingElement("Item")) {
                const char *key = item->Attribute("key");
                const char *val = item->Attribute("value");
                if(key && val) {
                    AllConfig[key] = val;
                }
            }
            for(tinyxml2::XMLElement *item =
                    rootElement->FirstChildElement("Custom");
                item; item = item->NextSiblingElement("Custom")) {
                const char *key = item->Attribute("key");
                const char *val = item->Attribute("value");
                if(key && val) {
                    CustomArguments.emplace_back(key, val);
                }
            }
            for(tinyxml2::XMLElement *item =
                    rootElement->FirstChildElement("KeyMap");
                item; item = item->NextSiblingElement("KeyMap")) {
                int key, val;
                if(tinyxml2::XML_SUCCESS ==
                       item->QueryIntAttribute("key", &key) &&
                   tinyxml2::XML_SUCCESS ==
                       item->QueryIntAttribute("value", &val) &&
                   key && val) {
                    KeyMap.emplace(key, val);
                }
            }
        }
    }
    if(fp)
        fclose(fp);
}

void iSysConfigManager::SaveToFile() {
    if(!ConfigUpdated)
        return;
    std::string filepath = GetFilePath();
    if(filepath.empty())
        return;
    tinyxml2::XMLDocument doc;
    doc.LinkEndChild(doc.NewDeclaration());
    tinyxml2::XMLElement *rootElement = doc.NewElement("GlobalPreference");
    for(auto it = AllConfig.begin(); it != AllConfig.end(); ++it) {
        tinyxml2::XMLElement *item = doc.NewElement("Item");
        item->SetAttribute("key", it->first.c_str());
        item->SetAttribute("value", it->second.c_str());
        rootElement->LinkEndChild(item);
    }
    for(auto it = CustomArguments.begin(); it != CustomArguments.end(); ++it) {
        tinyxml2::XMLElement *item = doc.NewElement("Custom");
        item->SetAttribute("key", it->first.c_str());
        item->SetAttribute("value", it->second.c_str());
        rootElement->LinkEndChild(item);
    }
    for(auto &it : KeyMap) {
        if(it.first && it.second) {
            tinyxml2::XMLElement *item = doc.NewElement("KeyMap");
            item->SetAttribute("key", it.first);
            item->SetAttribute("value", it.second);
            rootElement->LinkEndChild(item);
        }
    }
    doc.LinkEndChild(rootElement);
    XMLMemPrinter stream;
    doc.Print(&stream);
    stream.SaveFile(GetFilePath());
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
