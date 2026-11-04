#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace krkr::input {
    // Committed UTF-8 becomes the UTF-16 code units used by TJS. Malformed
    // input rejects the whole commit; no partially decoded prefix is emitted.
    // This does not represent IME composition, key codes, or focus routing.
    std::optional<std::u16string> decodeTextInput(std::string_view utf8);
} // namespace krkr::input
