//
// Created by LiDong on 2025/8/28.
//

#include <catch2/catch_test_macros.hpp>

#include "FontImpl.h"
#include "StorageImpl.h"
#include "SysInitImpl.h"
#include "Platform.h"
#include "test_config.h"

TEST_CASE("get default font name") {
    const std::string path = TVPGetAppStoragePath().front();
    TVPProjectDir = TVPNormalizeStorageName(path);

    TVPInitFontNames();
    const ttstr &fontName = TVPGetDefaultFontName();
    CAPTURE(fontName.c_str());
}
