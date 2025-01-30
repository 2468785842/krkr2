cmake_minimum_required(VERSION 3.16)
project(external)

include(./cmake/CocosExternalConfig.cmake)

add_library(external empty.cpp)

if (BUILD_EXT_BOX2D)
    add_subdirectory(Box2D)
    target_link_libraries(external PUBLIC ext_box2d)
endif (BUILD_EXT_BOX2D)
if (BUILD_EXT_CHIPMUNK)
    add_subdirectory(chipmunk)
    target_link_libraries(external PUBLIC ext_chipmunk)
endif (BUILD_EXT_CHIPMUNK)
if (BUILD_EXT_FREETYPE2)
    find_package(Freetype REQUIRED)
    target_link_libraries(external PUBLIC Freetype::Freetype)
endif (BUILD_EXT_FREETYPE2)
if (BUILD_EXT_RECAST)
    add_subdirectory(recast)
    target_link_libraries(external PUBLIC ext_recast)
endif (BUILD_EXT_RECAST)
if (BUILD_EXT_BULLET)
    add_subdirectory(bullet)
    target_link_libraries(external PUBLIC ext_bullet)
endif (BUILD_EXT_BULLET)
if (BUILD_EXT_JPEG)
    find_package(JPEG REQUIRED)
    target_link_libraries(external PUBLIC JPEG::JPEG)
endif (BUILD_EXT_JPEG)
if (BUILD_EXT_OPENSSL)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(external PUBLIC OpenSSL::SSL OpenSSL::Crypto)

    add_library(ext_ssl ALIAS OpenSSL::SSL)
    add_library(ext_crypto ALIAS OpenSSL::Crypto)
endif (BUILD_EXT_OPENSSL)
if (BUILD_EXT_TIFF)
    find_package(TIFF REQUIRED)
    target_link_libraries(external PUBLIC TIFF::TIFF)
endif (BUILD_EXT_TIFF)
if (BUILD_EXT_UV)
    add_subdirectory(uv)
    target_link_libraries(external PUBLIC ext_uv)
endif (BUILD_EXT_UV)
if (BUILD_EXT_WEBP)
    find_package(WebP CONFIG REQUIRED)
    target_link_libraries(external PUBLIC WebP::webp WebP::webpdecoder WebP::webpdemux)
endif (BUILD_EXT_WEBP)
if (BUILD_EXT_WEBSOCKETS)
    add_subdirectory(websockets)
    target_link_libraries(external PUBLIC ext_websockets)
endif (BUILD_EXT_WEBSOCKETS)
if (BUILD_EXT_TINYXML2)
    add_subdirectory(tinyxml2)
    target_link_libraries(external PUBLIC ext_tinyxml2)
endif (BUILD_EXT_TINYXML2)
if (BUILD_EXT_XXHASH)
    add_subdirectory(xxhash)
    target_link_libraries(external PUBLIC ext_xxhash)
endif (BUILD_EXT_XXHASH)
if (BUILD_EXT_XXTEA)
    add_subdirectory(xxtea)
    target_link_libraries(external PUBLIC ext_xxtea)
endif (BUILD_EXT_XXTEA)
if (BUILD_EXT_CLIPPER)
    add_subdirectory(clipper)
    target_link_libraries(external PUBLIC ext_clipper)
endif (BUILD_EXT_CLIPPER)
if (BUILD_EXT_EDTAA3FUNC)
    add_subdirectory(edtaa3func)
    target_link_libraries(external PUBLIC ext_edtaa3func)
endif (BUILD_EXT_EDTAA3FUNC)
if (BUILD_EXT_CONVERTUTF)
    add_subdirectory(ConvertUTF)
    target_link_libraries(external PUBLIC ext_convertUTF)
endif (BUILD_EXT_CONVERTUTF)
if (BUILD_EXT_POLY2TRI)
    add_subdirectory(poly2tri)
    target_link_libraries(external PUBLIC ext_poly2tri)
endif (BUILD_EXT_POLY2TRI)
if (BUILD_EXT_MD5)
    add_subdirectory(md5)
    target_link_libraries(external PUBLIC ext_md5)
endif (BUILD_EXT_MD5)

# use lua/js specific libs by property to prevent conflict
if (BUILD_JS_LIBS)
    add_subdirectory(spidermonkey)
    set_property(TARGET external APPEND PROPERTY
        CC_JS_DEPEND ext_spidermonkey
    )
endif ()
if (BUILD_LUA_LIBS)
    add_subdirectory(lua/luajit)
    add_subdirectory(lua/tolua)
    add_subdirectory(lua/luasocket)
    set_property(TARGET external APPEND PROPERTY
        CC_LUA_DEPEND ext_luajit ext_tolua ext_luasocket
    )
endif ()

if (NOT LINUX)
    if (BUILD_EXT_CURL)
        add_subdirectory(curl)
        target_link_libraries(external PUBLIC ext_curl)
    endif (BUILD_EXT_CURL)

    find_package(PNG REQUIRED)
    target_link_libraries(external PUBLIC PNG::PNG)

endif (NOT LINUX)

if (LINUX)
    add_subdirectory(linux-specific/fmod)
    target_link_libraries(external PUBLIC ext_fmod)
endif ()

if (ANDROID)
    add_subdirectory(android-specific/cpufeatures)
    add_subdirectory(android-specific/pvmp3dec)
    add_subdirectory(android-specific/tremolo)
    target_link_libraries(external PUBLIC ext_cpufeatures ext_pvmp3dec ext_tremolo)
endif ()

if (WINDOWS)
    # sqlite3 prebuilt only existed at windows
    if (BUILD_EXT_SQLITE)
        add_subdirectory(sqlite3)
        target_link_libraries(external PUBLIC ext_sqlite3)
    endif (BUILD_EXT_SQLITE)
    add_subdirectory(win32-specific/gles)
    add_subdirectory(win32-specific/icon)
    add_subdirectory(win32-specific/MP3Decoder)
    add_subdirectory(win32-specific/OggDecoder)
    add_subdirectory(win32-specific/OpenalSoft)
    target_link_libraries(external
        ext_gles
        ext_icon
        ext_MP3Decoder
        ext_OggDecoder
        ext_OpenalSoft
    )
endif ()

if (WINDOWS OR MACOSX OR LINUX)
    add_subdirectory(glfw3)
    target_link_libraries(external PUBLIC ext_glfw3)
endif ()

if (MACOSX OR ANDROID OR WINDOWS)
    if (BUILD_EXT_ZLIB)
        find_package(ZLIB REQUIRED)
        target_link_libraries(external PUBLIC ZLIB::ZLIB)
        add_library(ext_zlib ALIAS ZLIB::ZLIB)
    endif (BUILD_EXT_ZLIB)
endif ()

# unzip depend on zlib
if (BUILD_EXT_UNZIP)
    add_subdirectory(unzip)
    target_link_libraries(external PUBLIC ext_unzip)
endif (BUILD_EXT_UNZIP)

# put "external" into External folder, too
set_target_properties(external
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    FOLDER "External"
)
