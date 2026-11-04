#include "TextInput.h"

#include <cstdint>

namespace krkr::input {
    std::optional<std::u16string> decodeTextInput(std::string_view utf8) {
        std::u16string result;
        result.reserve(utf8.size());
        for(std::size_t offset = 0; offset < utf8.size();) {
            const auto lead = static_cast<unsigned char>(utf8[offset]);
            unsigned length;
            std::uint32_t scalar;
            if(lead <= 0x7f) {
                length = 1;
                scalar = lead;
            } else if(lead >= 0xc2 && lead <= 0xdf) {
                length = 2;
                scalar = lead & 0x1f;
            } else if(lead >= 0xe0 && lead <= 0xef) {
                length = 3;
                scalar = lead & 0x0f;
            } else if(lead >= 0xf0 && lead <= 0xf4) {
                length = 4;
                scalar = lead & 0x07;
            } else {
                return std::nullopt;
            }
            if(utf8.size() - offset < length)
                return std::nullopt;
            for(unsigned i = 1; i < length; ++i) {
                const auto next = static_cast<unsigned char>(utf8[offset + i]);
                if((next & 0xc0) != 0x80)
                    return std::nullopt;
                scalar = (scalar << 6) | (next & 0x3f);
            }
            if((length == 2 && scalar < 0x80) ||
               (length == 3 && scalar < 0x800) ||
               (length == 4 && scalar < 0x10000) || scalar > 0x10ffff ||
               (scalar >= 0xd800 && scalar <= 0xdfff))
                return std::nullopt;

            if(scalar <= 0xffff) {
                result.push_back(static_cast<char16_t>(scalar));
            } else {
                scalar -= 0x10000;
                result.push_back(
                    static_cast<char16_t>(0xd800 + (scalar >> 10)));
                result.push_back(
                    static_cast<char16_t>(0xdc00 + (scalar & 0x3ff)));
            }
            offset += length;
        }
        return result;
    }
} // namespace krkr::input
