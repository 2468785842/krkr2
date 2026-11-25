#include "ConfigFormat.h"
#include <nlohmann/json.hpp>
#include <pugixml.hpp>
#include <cerrno>
#include <cstdlib>
#include <limits>

namespace krkr::config {
    namespace {
        std::string NormalizeLegacyAttributes(std::string_view xml) {
            // Shipped locale files contain literal '<' in quoted attributes.
            // Keep that legacy value while giving the XML parser valid markup.
            std::string result;
            result.reserve(xml.size());
            bool markup = false;
            char quote = 0;
            for(std::size_t i = 0; i < xml.size(); ++i) {
                if(!markup) {
                    const auto skip = [&](std::string_view begin,
                                          std::string_view end) {
                        if(xml.substr(i, begin.size()) != begin)
                            return false;
                        const auto last = xml.find(end, i + begin.size());
                        const auto length = last == std::string_view::npos
                            ? xml.size() - i
                            : last + end.size() - i;
                        result.append(xml.substr(i, length));
                        i += length - 1;
                        return true;
                    };
                    if(skip("<!--", "-->") || skip("<![CDATA[", "]]>"))
                        continue;
                    if(xml[i] == '<')
                        markup = true;
                } else if(quote) {
                    if(xml[i] == quote)
                        quote = 0;
                    else if(xml[i] == '<') {
                        result += "&lt;";
                        continue;
                    } else if(xml[i] == '\t') {
                        result += "&#9;";
                        continue;
                    } else if(xml[i] == '\r' || xml[i] == '\n') {
                        // tinyxml2 preserves tabs and turns CR/LF pairs into
                        // one LF. References avoid XML attribute space folding.
                        const char newline = xml[i];
                        if(i + 1 < xml.size() &&
                           ((newline == '\r' && xml[i + 1] == '\n') ||
                            (newline == '\n' && xml[i + 1] == '\r')))
                            ++i;
                        result += "&#10;";
                        continue;
                    }
                } else if(xml[i] == '\'' || xml[i] == '"') {
                    quote = xml[i];
                } else if(xml[i] == '>') {
                    markup = false;
                }
                result += xml[i];
            }
            return result;
        }


        std::optional<std::string> Attribute(pugi::xml_node node,
                                             const char *key) {
            const auto attr = node.attribute(key);
            if(!attr)
                return std::nullopt;
            return std::string(attr.value());
        }
        std::optional<int> Integer(pugi::xml_node node, const char *key) {
            const auto text = Attribute(node, key);
            if(!text)
                return std::nullopt;
            char *end = nullptr;
            errno = 0;
            const auto value = std::strtol(text->c_str(), &end, 10);
            if(end == text->c_str() || errno == ERANGE ||
               value < std::numeric_limits<int>::min() ||
               value > std::numeric_limits<int>::max())
                return std::nullopt;
            return static_cast<int>(value);
        }


        using Json = nlohmann::json;
        bool StringValues(const Json &object, Values &values) {
            if(!object.is_object())
                return false;
            for(const auto &[key, value] : object.items()) {
                if(!value.is_string())
                    return false;
                values[key] = value.get<std::string>();
            }
            return true;
        }
    } // namespace

    std::optional<Preferences> ReadPreferences(std::string_view text) {
        const auto json = Json::parse(text, nullptr, false);
        if(!json.is_object() || !json.contains("version") ||
           json["version"] != 1)
            return std::nullopt;
        Preferences result;
        if(!json.contains("values") ||
           !StringValues(json["values"], result.values))
            return std::nullopt;
        if(json.contains("arguments")) {
            if(!json["arguments"].is_array())
                return std::nullopt;
            for(const auto &pair : json["arguments"]) {
                if(!pair.is_array() || pair.size() != 2 ||
                   !pair[0].is_string() || !pair[1].is_string())
                    return std::nullopt;
                result.arguments.emplace_back(pair[0].get<std::string>(),
                                              pair[1].get<std::string>());
            }
        }
        if(json.contains("keyMap")) {
            if(!json["keyMap"].is_array())
                return std::nullopt;
            for(const auto &pair : json["keyMap"]) {
                if(!pair.is_array() || pair.size() != 2)
                    return std::nullopt;
                int numbers[2];
                for(int i = 0; i < 2; ++i) {
                    const auto &value = pair[i];
                    if(!value.is_number_integer() ||
                       (value.is_number_unsigned() &&
                        value.get<std::uint64_t>() >
                            static_cast<std::uint64_t>(
                                std::numeric_limits<int>::max())) ||
                       (!value.is_number_unsigned() &&
                        (value.get<std::int64_t>() <
                             std::numeric_limits<int>::min() ||
                         value.get<std::int64_t>() >
                             std::numeric_limits<int>::max())))
                        return std::nullopt;
                    numbers[i] = value.get<int>();
                }
                if(numbers[0] && numbers[1])
                    result.keyMap.emplace(numbers[0], numbers[1]);
            }
        }
        return result;
    }

    std::string WritePreferences(const Preferences &preferences) {
        Json json = { { "version", 1 },
                      { "values", preferences.values },
                      { "arguments", preferences.arguments },
                      { "keyMap", Json::array() } };
        for(const auto &[key, value] : preferences.keyMap)
            if(key && value)
                json["keyMap"].push_back({ key, value });
        return json.dump(2) + "\n";
    }

    Values ReadLocale(std::string_view text) {
        const auto json = Json::parse(text, nullptr, false);
        Values result;
        if(!StringValues(json, result))
            return {};
        return result;
    }

    std::optional<Preferences> ReadLegacyPreferences(std::string_view xml) {
        pugi::xml_document doc;
        const auto normalized = NormalizeLegacyAttributes(xml);
        if(!doc.load_buffer(normalized.data(), normalized.size()) ||
           !doc.document_element())
            return std::nullopt;
        Preferences result;
        for(const auto node : doc.document_element().children()) {
            const std::string_view name = node.name();
            if(name == "Item" || name == "Custom") {
                const auto key = Attribute(node, "key");
                const auto value = Attribute(node, "value");
                if(key && value) {
                    if(name == "Item")
                        result.values[*key] = *value;
                    else
                        result.arguments.emplace_back(*key, *value);
                }
            } else if(name == "KeyMap") {
                const auto key = Integer(node, "key");
                const auto value = Integer(node, "value");
                if(key && value && *key && *value)
                    result.keyMap.emplace(*key, *value);
            }
        }
        return result;
    }

    Values ReadLegacyLocale(std::string_view xml) {
        pugi::xml_document doc;
        const auto normalized = NormalizeLegacyAttributes(xml);
        if(!doc.load_buffer(normalized.data(), normalized.size()))
            return {};
        Values result;
        for(const auto node : doc.document_element().children()) {
            const auto key = Attribute(node, "id");
            const auto value = Attribute(node, "text");
            if(key && value)
                result[*key] = *value;
        }
        return result;
    }
} // namespace krkr::config
