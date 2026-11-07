#include "ImageMetadata.h"

#include <charconv>
#include <string_view>

namespace PSB {

    bool tryParseTexIndex(const std::string &texName, std::uint32_t &index) {
        size_t texIdx = texName.rfind("tex");
        if(texIdx == std::string::npos)
            return false;

        const std::string_view remaining(texName.data() + texIdx,
                                         texName.size() - texIdx);
        const auto begin = remaining.find_first_of("0123456789");
        if(begin == std::string_view::npos)
            return false;
        auto end = remaining.find_first_not_of("0123456789", begin);
        if(end == std::string_view::npos)
            end = remaining.size();
        const auto result = std::from_chars(remaining.data() + begin,
                                            remaining.data() + end, index);
        return result.ec == std::errc{};
    }

    std::optional<std::uint32_t>
    ImageMetadata::getTextureIndex(const std::string &texName) {
        const auto endsWith = [&texName](std::string_view suffix) {
            return texName.size() >= suffix.size() &&
                texName.compare(texName.size() - suffix.size(), suffix.size(),
                                suffix.data(), suffix.size()) == 0;
        };
        if(endsWith("tex") || endsWith("tex#000") || endsWith("tex000")) {
            return 0;
        }

        std::uint32_t index;
        if(!tryParseTexIndex(texName, index)) {
            return {};
        }

        return index;
    }
} // namespace PSB