#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace krkr::config {
    using Values = std::unordered_map<std::string, std::string>;

    struct Preferences {
        Values values;
        std::vector<std::pair<std::string, std::string>> arguments;
        std::map<int, int> keyMap;
    };

    std::optional<Preferences> ReadPreferences(std::string_view json);
    std::string WritePreferences(const Preferences &preferences);
    std::optional<Preferences> ReadLegacyPreferences(std::string_view xml);
    Values ReadLegacyLocale(std::string_view xml);
    Values ReadLocale(std::string_view json);
} // namespace krkr::config
