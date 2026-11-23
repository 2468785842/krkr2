#include "ConfigXml.h"

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <cerrno>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>

namespace krkr::config {
    namespace {
        using Document = std::unique_ptr<xmlDoc, decltype(&xmlFreeDoc)>;
        struct FreeXml {
            void operator()(xmlChar *text) const { xmlFree(text); }
        };
        using Text = std::unique_ptr<xmlChar, FreeXml>;

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

        Document Parse(std::string_view xml) {
            if(xml.empty() || xml.size() > std::numeric_limits<int>::max())
                return { nullptr, xmlFreeDoc };
            const auto normalized = NormalizeLegacyAttributes(xml);
            if(normalized.size() > std::numeric_limits<int>::max())
                return { nullptr, xmlFreeDoc };
            return { xmlReadMemory(normalized.data(),
                                   static_cast<int>(normalized.size()), nullptr,
                                   nullptr,
                                   XML_PARSE_NONET | XML_PARSE_NOERROR |
                                       XML_PARSE_NOWARNING),
                     xmlFreeDoc };
        }

        std::optional<std::string> Attribute(xmlNode *node, const char *key) {
            Text text(xmlGetProp(node, BAD_CAST key));
            if(!text)
                return std::nullopt;
            return std::string(reinterpret_cast<const char *>(text.get()));
        }

        bool Named(xmlNode *node, const char *name) {
            return node->type == XML_ELEMENT_NODE &&
                xmlStrEqual(node->name, BAD_CAST name);
        }

        std::optional<int> Integer(xmlNode *node, const char *key) {
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

        void SetAttribute(xmlNode *node, const char *key,
                          const std::string &value) {
            if(!xmlNewProp(node, BAD_CAST key, BAD_CAST value.c_str()))
                throw std::bad_alloc();
        }

        void AddItem(xmlNode *root, const char *name, const std::string &key,
                     const std::string &value) {
            auto *node = xmlNewChild(root, nullptr, BAD_CAST name, nullptr);
            if(!node)
                throw std::bad_alloc();
            SetAttribute(node, "key", key);
            SetAttribute(node, "value", value);
        }
    } // namespace

    std::optional<Preferences> ReadPreferences(std::string_view xml) {
        const auto doc = Parse(xml);
        auto *root = doc ? xmlDocGetRootElement(doc.get()) : nullptr;
        if(!root)
            return std::nullopt;
        Preferences result;
        for(auto *node = root->children; node; node = node->next) {
            if(Named(node, "Item") || Named(node, "Custom")) {
                const auto key = Attribute(node, "key");
                const auto value = Attribute(node, "value");
                if(key && value) {
                    if(Named(node, "Item"))
                        result.values[*key] = *value;
                    else
                        result.arguments.emplace_back(*key, *value);
                }
            } else if(Named(node, "KeyMap")) {
                const auto key = Integer(node, "key");
                const auto value = Integer(node, "value");
                if(key && value && *key && *value)
                    result.keyMap.emplace(*key, *value);
            }
        }
        return result;
    }

    std::string WritePreferences(const Preferences &preferences) {
        Document doc(xmlNewDoc(BAD_CAST "1.0"), xmlFreeDoc);
        if(!doc)
            throw std::bad_alloc();
        auto *root = xmlNewNode(nullptr, BAD_CAST "GlobalPreference");
        if(!root)
            throw std::bad_alloc();
        xmlDocSetRootElement(doc.get(), root);
        for(const auto &[key, value] : preferences.values)
            AddItem(root, "Item", key, value);
        for(const auto &[key, value] : preferences.arguments)
            AddItem(root, "Custom", key, value);
        for(const auto &[key, value] : preferences.keyMap)
            if(key && value)
                AddItem(root, "KeyMap", std::to_string(key),
                        std::to_string(value));
        xmlChar *bytes = nullptr;
        int size = 0;
        xmlDocDumpFormatMemoryEnc(doc.get(), &bytes, &size, "UTF-8", 1);
        Text owned(bytes);
        if(!owned)
            throw std::bad_alloc();
        return { reinterpret_cast<const char *>(bytes),
                 static_cast<std::size_t>(size) };
    }

    Values ReadLocale(std::string_view xml) {
        Values result;
        const auto doc = Parse(xml);
        auto *root = doc ? xmlDocGetRootElement(doc.get()) : nullptr;
        if(root) {
            for(auto *node = root->children; node; node = node->next) {
                if(node->type != XML_ELEMENT_NODE)
                    continue;
                const auto key = Attribute(node, "id");
                const auto value = Attribute(node, "text");
                if(key && value)
                    result[*key] = *value;
            }
        }
        return result;
    }
} // namespace krkr::config
