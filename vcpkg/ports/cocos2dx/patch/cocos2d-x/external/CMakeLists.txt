cmake_minimum_required(VERSION 3.16)
project(external)

include(./cmake/CocosExternalConfig.cmake)

add_library(external INTERFACE)

if (BUILD_EXT_BOX2D)
    find_package(box2d CONFIG REQUIRED)
    target_link_libraries(external INTERFACE box2d::box2d)
endif (BUILD_EXT_BOX2D)
if (BUILD_EXT_CHIPMUNK)
    find_package(unofficial-chipmunk CONFIG REQUIRED)

    target_link_libraries(external INTERFACE
            unofficial::chipmunk::chipmunk
    )
endif (BUILD_EXT_CHIPMUNK)
if (BUILD_EXT_FREETYPE2)
    find_package(Freetype REQUIRED)
    target_link_libraries(external INTERFACE Freetype::Freetype)
endif (BUILD_EXT_FREETYPE2)
if (BUILD_EXT_RECAST)
    add_subdirectory(recast) # source code
    target_link_libraries(external INTERFACE ext_recast)
endif (BUILD_EXT_RECAST)
if (BUILD_EXT_BULLET)
    find_package(Bullet CONFIG REQUIRED)
    target_link_libraries(external INTERFACE BulletDynamics BulletCollision LinearMath)
endif (BUILD_EXT_BULLET)
if (BUILD_EXT_JPEG)
    find_package(JPEG REQUIRED)
    target_link_libraries(external INTERFACE JPEG::JPEG)
endif (BUILD_EXT_JPEG)
if (BUILD_EXT_OPENSSL)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(external INTERFACE OpenSSL::SSL OpenSSL::Crypto)

    add_library(ext_ssl ALIAS OpenSSL::SSL)
    add_library(ext_crypto ALIAS OpenSSL::Crypto)
endif (BUILD_EXT_OPENSSL)
if (BUILD_EXT_TIFF)
    find_package(TIFF REQUIRED)
    target_link_libraries(external INTERFACE TIFF::TIFF)
endif (BUILD_EXT_TIFF)
if (BUILD_EXT_UV)
    find_package(libuv CONFIG REQUIRED)
    target_link_libraries(external INTERFACE $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>)
endif (BUILD_EXT_UV)
if (BUILD_EXT_WEBP)
    find_package(WebP CONFIG REQUIRED)
    target_link_libraries(external INTERFACE WebP::webp WebP::webpdecoder WebP::webpdemux)
endif (BUILD_EXT_WEBP)
if (BUILD_EXT_WEBSOCKETS)
    find_package(libwebsockets CONFIG REQUIRED)
    target_include_directories(external INTERFACE ${LIBWEBSOCKETS_INCLUDE_DIRS})
    target_link_libraries(external INTERFACE websockets)
endif (BUILD_EXT_WEBSOCKETS)
if (BUILD_EXT_TINYXML2)
    add_subdirectory(tinyxml2) # source code
    target_link_libraries(external INTERFACE ext_tinyxml2)
endif (BUILD_EXT_TINYXML2)
if (BUILD_EXT_XXHASH)
    add_subdirectory(xxhash) # source code
    target_link_libraries(external INTERFACE ext_xxhash)
endif (BUILD_EXT_XXHASH)
if (BUILD_EXT_XXTEA)
    add_subdirectory(xxtea) # source code
    target_link_libraries(external INTERFACE ext_xxtea)
endif (BUILD_EXT_XXTEA)
if (BUILD_EXT_CLIPPER)
    add_subdirectory(clipper) # source code
    target_link_libraries(external INTERFACE ext_clipper)
endif (BUILD_EXT_CLIPPER)
if (BUILD_EXT_EDTAA3FUNC)
    add_subdirectory(edtaa3func) # source code
    target_link_libraries(external INTERFACE ext_edtaa3func)
endif (BUILD_EXT_EDTAA3FUNC)
if (BUILD_EXT_CONVERTUTF)
    add_subdirectory(ConvertUTF) # source code
    target_link_libraries(external INTERFACE ext_convertUTF)
endif (BUILD_EXT_CONVERTUTF)
if (BUILD_EXT_POLY2TRI)
    add_subdirectory(poly2tri) # source code
    target_link_libraries(external INTERFACE ext_poly2tri)
endif (BUILD_EXT_POLY2TRI)
if (BUILD_EXT_MD5)
    add_subdirectory(md5) # source code
    target_link_libraries(external INTERFACE ext_md5)
endif (BUILD_EXT_MD5)

if (BUILD_EXT_CURL)
    find_package(CURL REQUIRED)
    target_link_libraries(external INTERFACE CURL::libcurl)
endif (BUILD_EXT_CURL)

find_package(PNG REQUIRED)
target_link_libraries(external INTERFACE PNG::PNG)


if (LINUX)
    add_subdirectory(linux-specific/fmod)
    target_link_libraries(external INTERFACE ext_fmod) # will replace
endif ()

if (ANDROID)

    find_package(Ogg CONFIG REQUIRED)
    find_package(Vorbis CONFIG REQUIRED)

    add_subdirectory(android-specific/cpufeatures) # source code
    add_subdirectory(android-specific/pvmp3dec) # source code
    target_link_libraries(external INTERFACE
            ext_cpufeatures ext_pvmp3dec
            Ogg::ogg
            Vorbis::vorbis
            Vorbis::vorbisfile
            Vorbis::vorbisenc)

    install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/android-specific/tremolo"
            DESTINATION "include/Tremolo"
            FILES_MATCHING PATTERN "ivorbisfile.h"
    )

    target_include_directories(external
            INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/android-specific/tremolo>
                $<INSTALL_INTERFACE:include>)
endif ()

if (WINDOWS)
    # sqlite3 prebuilt only existed at windows
    if (BUILD_EXT_SQLITE)
        find_package(unofficial-sqlite3 CONFIG REQUIRED)
        target_link_libraries(external INTERFACE unofficial::sqlite3::sqlite3)
    endif (BUILD_EXT_SQLITE)

    find_package(GLEW REQUIRED)
    find_package(Iconv)
    find_package(OpenAL CONFIG REQUIRED)
    find_package(mpg123 CONFIG REQUIRED)
    find_package(Ogg CONFIG REQUIRED)

    target_link_libraries(external INTERFACE
            GLEW::GLEW
            Iconv::Iconv
            MPG123::libmpg123
            Ogg::ogg
            OpenAL::OpenAL
    )

endif ()

if (WINDOWS OR MACOSX OR LINUX)
    find_package(glfw3 CONFIG REQUIRED)
    target_link_libraries(external INTERFACE glfw)
endif ()

if (BUILD_EXT_ZLIB)
    find_package(ZLIB REQUIRED)
    target_link_libraries(external INTERFACE ZLIB::ZLIB)
    add_library(ext_zlib ALIAS ZLIB::ZLIB)
endif (BUILD_EXT_ZLIB)

# unzip depend on zlib
if (BUILD_EXT_UNZIP)
    add_subdirectory(unzip) # source code
    target_link_libraries(external INTERFACE ext_unzip)
endif (BUILD_EXT_UNZIP)
