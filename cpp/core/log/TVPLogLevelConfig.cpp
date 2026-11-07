#include "TVPLogLevelConfig.h"

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <spdlog/spdlog.h>

namespace {

    struct LogLevelFilter {
        bool configured = false;
        bool globalDisabled = false;
        TVPLogLevel defaultMin = TVPLogLevel::Trace;
        std::unordered_map<std::string, TVPLogLevel> categoryLevels;
        std::unordered_set<std::string> disabledCategories;
    };

    LogLevelFilter &filterConfig() {
        static LogLevelFilter config;
        return config;
    }

    int levelRank(TVPLogLevel level) {
        switch(level) {
            case TVPLogLevel::Trace:
                return 0;
            case TVPLogLevel::Debug:
                return 1;
            case TVPLogLevel::Info:
                return 2;
            case TVPLogLevel::Warn:
                return 3;
            case TVPLogLevel::Error:
                return 4;
            case TVPLogLevel::Fatal:
                return 5;
        }
        return 2;
    }

    std::string trimCopy(std::string value) {
        while(!value.empty() &&
              std::isspace(static_cast<unsigned char>(value.front())))
            value.erase(value.begin());
        while(!value.empty() &&
              std::isspace(static_cast<unsigned char>(value.back())))
            value.pop_back();
        return value;
    }

    std::string toLowerCopy(std::string value) {
        for(char &ch : value)
            ch =
                static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
        return value;
    }

    enum class ParsedLevel { Level, Off, Invalid };

    ParsedLevel parseLevelName(const std::string &name, TVPLogLevel &out) {
        const std::string level = toLowerCopy(trimCopy(name));
        if(level == "trace")
            out = TVPLogLevel::Trace;
        else if(level == "debug")
            out = TVPLogLevel::Debug;
        else if(level == "info")
            out = TVPLogLevel::Info;
        else if(level == "warn" || level == "warning")
            out = TVPLogLevel::Warn;
        else if(level == "error" || level == "err")
            out = TVPLogLevel::Error;
        else if(level == "fatal" || level == "critical")
            out = TVPLogLevel::Fatal;
        else if(level == "off")
            return ParsedLevel::Off;
        else
            return ParsedLevel::Invalid;
        return ParsedLevel::Level;
    }

    spdlog::level::level_enum toSpdlogLevel(TVPLogLevel level) {
        switch(level) {
            case TVPLogLevel::Trace:
                return spdlog::level::trace;
            case TVPLogLevel::Debug:
                return spdlog::level::debug;
            case TVPLogLevel::Info:
                return spdlog::level::info;
            case TVPLogLevel::Warn:
                return spdlog::level::warn;
            case TVPLogLevel::Error:
                return spdlog::level::err;
            case TVPLogLevel::Fatal:
                return spdlog::level::critical;
        }
        return spdlog::level::info;
    }

    bool isCategoryDisabled(const char *category) {
        const auto &config = filterConfig();
        if(config.globalDisabled)
            return true;
        if(!category)
            category = "core";
        return config.disabledCategories.count(category) != 0;
    }

    TVPLogLevel minLevelForCategory(const char *category) {
        const auto &config = filterConfig();
        if(!category)
            category = "core";
        const auto it = config.categoryLevels.find(category);
        if(it != config.categoryLevels.end())
            return it->second;
        return config.defaultMin;
    }

    void applySpdlogLoggerLevel(const char *name) {
        auto logger = spdlog::get(name);
        if(!logger)
            return;
        if(isCategoryDisabled(name)) {
            logger->set_level(spdlog::level::off);
            return;
        }
        logger->set_level(toSpdlogLevel(minLevelForCategory(name)));
    }

    void applySpdlogLevels() {
        static const char *kKnownLoggers[] = {
            "core", "tjs2", "plugin", "engine", "exception", "xp3",
        };
        for(const char *name : kKnownLoggers)
            applySpdlogLoggerLevel(name);
        const auto &config = filterConfig();
        for(const auto &entry : config.categoryLevels)
            applySpdlogLoggerLevel(entry.first.c_str());
        for(const auto &name : config.disabledCategories)
            applySpdlogLoggerLevel(name.c_str());
    }

} // namespace

void TVPApplySpdlogLoggerLevel(const char *category) {
    applySpdlogLoggerLevel(category);
}

namespace {

    void parseLevelSpec(const char *spec, TVPLogLevel defaultMinLevel) {
        auto &config = filterConfig();
        config = LogLevelFilter{};
        config.defaultMin = defaultMinLevel;

        if(!spec || !*spec)
            return;

        config.configured = true;
        const std::string value = trimCopy(spec);
        if(value.find(':') == std::string::npos) {
            TVPLogLevel level = defaultMinLevel;
            switch(parseLevelName(value, level)) {
                case ParsedLevel::Level:
                    config.defaultMin = level;
                    break;
                case ParsedLevel::Off:
                    config.globalDisabled = true;
                    break;
                case ParsedLevel::Invalid:
                    config.configured = false;
                    return;
            }
            applySpdlogLevels();
            return;
        }

        std::string entry;
        for(size_t i = 0; i <= value.size(); ++i) {
            if(i == value.size() || value[i] == ',') {
                entry = trimCopy(entry);
                if(!entry.empty()) {
                    const size_t sep = entry.find(':');
                    if(sep != std::string::npos) {
                        const std::string category =
                            toLowerCopy(trimCopy(entry.substr(0, sep)));
                        TVPLogLevel level = defaultMinLevel;
                        switch(parseLevelName(entry.substr(sep + 1), level)) {
                            case ParsedLevel::Level:
                                config.categoryLevels[category] = level;
                                break;
                            case ParsedLevel::Off:
                                config.disabledCategories.insert(category);
                                break;
                            case ParsedLevel::Invalid:
                                break;
                        }
                    }
                }
                entry.clear();
                continue;
            }
            entry.push_back(value[i]);
        }

        applySpdlogLevels();
    }

} // namespace

bool TVPIsLogEnabled(TVPLogLevel level, const char *category) {
    const auto &config = filterConfig();
    if(!config.configured)
        return true;
    if(isCategoryDisabled(category))
        return false;
    return levelRank(level) >= levelRank(minLevelForCategory(category));
}

void TVPApplyLogLevelSpec(const char *spec, TVPLogLevel defaultMinLevel) {
    parseLevelSpec(spec, defaultMinLevel);
}
