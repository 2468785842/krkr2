#pragma once

#include <map>
#include <string>

namespace krkr::launch {
    void AddDesktopSetting(std::map<std::string, std::string> &settings,
                           const std::string &assignment);
    std::string ListDesktopSettings();
} // namespace krkr::launch
