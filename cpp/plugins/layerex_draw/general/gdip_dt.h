//
// Created by LiDong on 2025/9/19.
//
#pragma once

// because on MacOS the BOOL redefined in objc.h
// libgdiplus has BOOL too!

#if TARGET_OS_MAC || TARGET_OS_IPHONE
#define _WAPI_UGLIFY_H_
#define HANDLE void *
#define BOOL bool
#define CONST const
#define VOID void
#endif
extern "C" {
#include <libgdiplus/gdiplus-private.h>
#include <libgdiplus/gdipenums.h>
#include <libgdiplus/bitmap-private.h>
#include <libgdiplus/graphics-private.h>
#include <libgdiplus/graphics-path-private.h>
#include <libgdiplus/customlinecap-private.h>
#include <libgdiplus/matrix-private.h>
#include <libgdiplus/image-private.h>
#include <libgdiplus/pen-private.h>
#include <libgdiplus/customlinecap-private.h>
#include <libgdiplus/fontfamily-private.h>
#include <libgdiplus/fontcollection-private.h>
#include <libgdiplus/font-private.h>
#include <libgdiplus/adjustablearrowcap-private.h>
#include <libgdiplus/gdiplus-private.h>
#include <libgdiplus/pen-private.h>
#include <libgdiplus/gdiplus-private.h>
#include <libgdiplus/brush-private.h>
#include <libgdiplus/solidbrush-private.h>
#include <libgdiplus/hatchbrush-private.h>
#include <libgdiplus/texturebrush-private.h>
#include <libgdiplus/pathgradientbrush-private.h>
#include <libgdiplus/lineargradientbrush-private.h>
}
#undef HANDLE
#undef BOOL
#undef CONST
#undef VOID