#include "ImageMetadata.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>

namespace PSB {

    bool tryParseTexIndex(const std::string &texName, std::uint32_t &index) {
        size_t texIdx = texName.rfind("tex");
        if(texIdx == std::string::npos)
            return false;

        std::string remaining = texName.substr(texIdx);

        boost::smatch match;
        if(boost::regex_search(remaining, match, boost::regex("\\d+"))) {
            try {
                index = boost::lexical_cast<std::uint32_t>(match[0]);
                return true;
            } catch(const boost::bad_lexical_cast &) {
                return false;
            }
        }

        return false;
    }

    std::optional<std::uint32_t>
    ImageMetadata::getTextureIndex(const std::string &texName) {
        using namespace boost::algorithm;
        if(ends_with(texName, "tex") || ends_with(texName, "tex#000") ||
           ends_with(texName, "tex000")) {
            return 0;
        }

        std::uint32_t index;
        if(!tryParseTexIndex(texName, index)) {
            return {};
        }

        return index;
    }
} // namespace PSB