//
// Created by LiDon on 2025/6/16.
//
#pragma once

#include <string>

namespace PSB::Consts {

    // The string with this prefix (with ID followed) will be convert to
    // resource when compile/decompile
    static inline const std::string &ResourceIdentifier = "#resource#";

    // The string with this prefix (with ID followed) will be convert to extra
    // resource when compile/decompile
    static inline const std::string &ExtraResourceIdentifier = "#resource@";

} // namespace PSB::Consts
