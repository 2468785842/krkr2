cmake_minimum_required(VERSION 3.28)
project(libgdiplus VERSION 5.6.1 LANGUAGES C)

# 配置选项
option(WITH_PANGO "Use Pango for text rendering" ON)
option(WITH_LIBEXIF "Enable EXIF support" ON)
option(WITH_X11 "Enable X11 support" OFF)

add_library(${PROJECT_NAME})

add_definitions(-DHAVE_CONFIG_H -DVERSION="${PROJECT_VERSION}")
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

# 检查系统字节序
include(TestBigEndian)
test_big_endian(WORDS_BIGENDIAN)
if(WORDS_BIGENDIAN)
    add_definitions(-DWORDS_BIGENDIAN=1)
endif()

# 查找核心依赖
find_package(PkgConfig REQUIRED)

pkg_check_modules(GLIB REQUIRED glib-2.0>=2.2.3)
pkg_check_modules(CAIRO REQUIRED cairo>=1.6.4)
pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
pkg_check_modules(FREETYPE REQUIRED freetype2)

if(NOT GLIB_FOUND)
    message(FATAL_ERROR "glib-2.0 not found")
endif()

if(NOT CAIRO_FOUND)
    message(FATAL_ERROR "cairo not found")
endif()

if(NOT FONTCONFIG_FOUND)
    message(FATAL_ERROR "fontconfig not found")
endif()

if(NOT FREETYPE_FOUND)
    message(FATAL_ERROR "freetype2 not found")
endif()

include(CheckSymbolExists)
list(APPEND CMAKE_REQUIRED_INCLUDES ${FONTCONFIG_INCLUDE_DIRS})
list(APPEND CMAKE_REQUIRED_LIBRARIES ${FONTCONFIG_LIBRARIES})

# version 3.31+
#list(APPEND CMAKE_REQUIRED_LINK_DIRECTORIES ${FONTCONFIG_LIBRARY_DIRS})
foreach (FONTCONFIG_LIBRARY_DIR ${FONTCONFIG_LIBRARY_DIRS})
    if(MSVC)
        list(APPEND CMAKE_REQUIRED_LINK_OPTIONS "/LIBPATH:${FONTCONFIG_LIBRARY_DIR}")
    else()
        list(APPEND CMAKE_REQUIRED_LINK_OPTIONS "-L${FONTCONFIG_LIBRARY_DIR}")
    endif()
endforeach()

check_symbol_exists(FcFini "fontconfig/fontconfig.h" HAVE_FCFINI)

# Pango可选支持
if(WITH_PANGO)
    pkg_check_modules(PANGO REQUIRED pango>=1.10)
    set(USE_PANGO_RENDERING ON)
    list(APPEND GDIPLUS_LIBS ${PANGO_LIBRARIES})
    list(APPEND GDIPLUS_CFLAGS ${PANGO_CFLAGS})
    target_include_directories(${PROJECT_NAME} PUBLIC ${PANGO_INCLUDE_DIRS})
endif()

# 图像编解码库
## JPEG
find_package(JPEG)
if(JPEG_FOUND)
    set(HAVE_LIBJPEG ON)
    list(APPEND GDIPLUS_LIBS ${JPEG_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PUBLIC ${JPEG_INCLUDE_DIRS})
else()
    message(WARNING "JPEG support disabled")
endif()

## TIFF
find_package(TIFF)
if(TIFF_FOUND)
    set(HAVE_LIBTIFF ON)
    list(APPEND GDIPLUS_LIBS ${TIFF_LIBRARIES})
    target_include_directories(${PROJECT_NAME} PUBLIC ${TIFF_INCLUDE_DIRS})
else()
    message(WARNING "TIFF support disabled")
endif()

## GIF
#find_library(GIF_LIBRARY NAMES gif ungif)
#find_path(GIF_INCLUDE_DIR gif_lib.h)
#if(GIF_LIBRARY AND GIF_INCLUDE_DIR)
#    set(HAVE_LIBGIF ON)
#    list(APPEND GDIPLUS_LIBS ${GIF_LIBRARY})
#    target_include_directories(${PROJECT_NAME} PUBLIC ${GIF_INCLUDE_DIR})
#else()
#    message(WARNING "GIF support disabled")
#endif()

## PNG
find_package(PNG REQUIRED)
set(HAVE_LIBPNG ON)
list(APPEND GDIPLUS_LIBS ${PNG_LIBRARIES})

# X11支持
if(WITH_X11)
    find_package(X11 REQUIRED)
    if(X11_FOUND)
        set(HAVE_X11 ON)
        pkg_check_modules(XRENDER REQUIRED xrender)
        list(APPEND GDIPLUS_LIBS ${X11_LIBRARIES} ${XRENDER_LIBRARIES})
        list(APPEND GDIPLUS_CFLAGS ${XRENDER_CFLAGS})
    else()
        message(FATAL_ERROR "X11 not find")
    endif()
endif()

# libexif支持
if(WITH_LIBEXIF)
    pkg_check_modules(LIBEXIF libexif)
    if(LIBEXIF_FOUND)
        set(HAVE_LIBEXIF ON)
        list(APPEND GDIPLUS_LIBS ${LIBEXIF_LIBRARIES})
    else()
        message(WARNING "EXIF support disabled")
    endif()
endif()

# 平台特定设置
if(WINDOWS)
    add_definitions(-DWIN32=1)
    find_library(INTL_LIBRARY NAMES intl)
    list(APPEND GDIPLUS_LIBS ${INTL_LIBRARY})
elseif(APPLE)
    add_definitions(-DOSX=1)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -no-cpp-precomp")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flat_namespace -undefined suppress")
#elseif(LINUX)
#    find_package(Threads REQUIRED)
#    list(APPEND GDIPLUS_LIBS Threads::Threads)
endif()

target_sources(${PROJECT_NAME}
    PRIVATE
        src/adjustablearrowcap.c
        src/bitmap.c
        src/brush.c
        src/carbon-private.c
        src/customlinecap.c
        src/dstream.c
        src/font.c
        src/gdi32.c
        src/general.c
        src/graphics.c
        src/graphics-cairo.c
        src/graphics-metafile.c
        src/graphics-path.c
        src/graphics-pathiterator.c
        src/hatchbrush.c
        src/icocodec.c
        src/image.c
        src/imageattributes.c
        src/lineargradientbrush.c
        src/matrix.c
        src/metafile.c
        src/pathgradientbrush.c
        src/pen.c
        src/print.c
        src/region.c
        src/region-bitmap.c
        src/region-path-tree.c
        src/solidbrush.c
        src/stringformat.c
        src/text.c
        src/text-cairo.c
        src/text-pango.c
        src/text-metafile.c
        src/texturebrush.c
        src/bmpcodec.c
        src/emfcodec.c
        src/emfplus.c
        src/gifcodec.c
        src/jpegcodec.c
        src/pngcodec.c
        src/tiffcodec.c
        src/wmfcodec.c
)

# 合并所有依赖
target_include_directories(${PROJECT_NAME} PUBLIC
    ${GLIB_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${FONTCONFIG_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PUBLIC
    ${GLIB_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${GDIPLUS_LIBS}
)

target_link_directories(${PROJECT_NAME} PUBLIC
    ${GLIB_LIBRARY_DIRS}
    ${CAIRO_LIBRARY_DIRS}
    ${FONTCONFIG_LIBRARY_DIRS}
    ${FREETYPE_LIBRARY_DIRS}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>
)

configure_file(config.h.in config.h)

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/src/"
    DESTINATION "include/libgdiplus"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.inc"
)

install(
    TARGETS libgdiplus
    EXPORT libgdiplus-config
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(
    EXPORT libgdiplus-config
    DESTINATION share/libgdiplus
    NAMESPACE libgdiplus::
)
