#/****************************************************************************
# Copyright (c) 2013 cocos2dx-x.org
# Copyright (c) 2014 martell malone
# Copyright (c) 2015-2017 Chukong Technologies Inc.
#
# http://www.cocos2d-x.org
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# ****************************************************************************/

# build engine library and all tests

cmake_minimum_required(VERSION 3.6)

project(Cocos2d-x)

# cocos2dx root path
set(COCOS2DX_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
set(CMAKE_MODULE_PATH ${COCOS2DX_ROOT_PATH}/cmake/Modules/)

# prevent in-source-build
include(PreventInSourceBuilds)

# works before build libcocos2d
include(CocosBuildSet)

# build options
option(BUILD_TESTS "Build tests" ON)

option(BUILD_EXT_BOX2D "Build with internal Box2D support" ON)
option(BUILD_EXT_CHIPMUNK "Build with internal chipmunk support" ON)
option(BUILD_EXT_FREETYPE2 "Build with internal freetype2 support" ON)
option(BUILD_EXT_RECAST "Build with internal recast support" ON)
option(BUILD_EXT_BULLET "Build with internal bullet support" ON)
option(BUILD_EXT_JPEG "Build with internal jpeg support" ON)
option(BUILD_EXT_OPENSSL "Build with internal openssl support" ON)
option(BUILD_EXT_TIFF "Build with internal tiff support" ON)
option(BUILD_EXT_UV "Build with internal uv support" ON)
option(BUILD_EXT_WEBP "Build with internal webp support" ON)
option(BUILD_EXT_WEBSOCKETS "Build with internal websockets support" ON)
option(BUILD_EXT_TINYXML2 "Build with internal tinyxml2 support" ON)
option(BUILD_EXT_XXHASH "Build with internal xxhash support" ON)
option(BUILD_EXT_XXTEA "Build with internal xxtea support" ON)
option(BUILD_EXT_CLIPPER "Build with internal clipper support" ON)
option(BUILD_EXT_EDTAA3FUNC "Build with internal edtaa3func support" ON)
option(BUILD_EXT_CONVERTUTF "Build with internal ConvertUTF support" ON)
option(BUILD_EXT_POLY2TRI "Build with internal poly2tri support" ON)
option(BUILD_EXT_MD5 "Build with internal md5 support" ON)
option(BUILD_EXT_SQLITE "Build with internal SQLite support" ON)
option(BUILD_EXT_ZLIB "Build with internal zlib support" ON)
option(BUILD_EXT_CURL "Build with internal curl support" ON)
option(BUILD_EXT_UNZIP "Build with internal unzip support" ON)

# default tests include lua, js test project, so we set those option on to build libs
option(BUILD_LUA_LIBS ON)
option(BUILD_JS_LIBS ON)

add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos ${ENGINE_BINARY_PATH}/cocos/core)

if (ANDROID)
    add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos/platform/android ${ENGINE_BINARY_PATH}/cocos/android)
endif ()

# prevent tests project to build "cocos2dx-x/cocos" again
set(BUILD_ENGINE_DONE ON)
# add engine all tests project
if (BUILD_TESTS)
    add_subdirectory(${COCOS2DX_ROOT_PATH}/tests/cpp-empty-test ${ENGINE_BINARY_PATH}/tests/cpp-empty-test)
    add_subdirectory(${COCOS2DX_ROOT_PATH}/tests/cpp-tests ${ENGINE_BINARY_PATH}/tests/cpp-tests)
    add_subdirectory(${COCOS2DX_ROOT_PATH}/tests/js-tests/project ${ENGINE_BINARY_PATH}/tests/js-tests)
    add_subdirectory(${COCOS2DX_ROOT_PATH}/tests/lua-empty-test/project ${ENGINE_BINARY_PATH}/tests/lua-empty-test)
    add_subdirectory(${COCOS2DX_ROOT_PATH}/tests/lua-tests/project ${ENGINE_BINARY_PATH}/tests/lua-test)
endif ()

set(INSTALL_TARGETS cocos2d external)

if (BUILD_EXT_UNZIP)
    list(APPEND INSTALL_TARGETS ext_unzip)
endif ()
if (BUILD_EXT_TINYXML2)
    list(APPEND INSTALL_TARGETS ext_tinyxml2)
endif ()
if (BUILD_EXT_XXHASH)
    list(APPEND INSTALL_TARGETS ext_xxhash)
endif ()
if(BUILD_EXT_XXTEA)
    list(APPEND INSTALL_TARGETS ext_xxtea)
endif()
if (BUILD_EXT_EDTAA3FUNC)
    list(APPEND INSTALL_TARGETS ext_edtaa3func)
endif ()
if (BUILD_EXT_POLY2TRI)
    list(APPEND INSTALL_TARGETS ext_poly2tri)
endif ()
if (BUILD_EXT_RECAST)
    list(APPEND INSTALL_TARGETS ext_recast)
endif ()
if (BUILD_EXT_CLIPPER)
    list(APPEND INSTALL_TARGETS ext_clipper)
endif ()
if (BUILD_EXT_CONVERTUTF)
    list(APPEND INSTALL_TARGETS ext_convertUTF)
endif ()
if (ANDROID)
    list(APPEND INSTALL_TARGETS ext_cpufeatures ext_pvmp3dec cpp_android_spec)
endif ()
if (BUILD_EXT_MD5)
    list(APPEND INSTALL_TARGETS ext_md5)
endif ()

macro(install_ext_lib absolute_path)
    install(
        DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/external/${absolute_path}/"
        DESTINATION "share/cocos2dx/${absolute_path}"
        FILES_MATCHING PATTERN "*.a" PATTERN "*.lib" PATTERN "*.dll" PATTERN "*.so" PATTERN "*.h" PATTERN "*.hpp"
    )
endmacro()

if(LINUX)
    install_ext_lib("linux-specific/fmod")
endif()

install(
    TARGETS ${INSTALL_TARGETS}
    EXPORT cocos2dx-targets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cocos2dx-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cocos2dx-config.cmake"
    INSTALL_DESTINATION "share/cocos2dx"
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/cocos2dx-config.cmake"
    DESTINATION "share/cocos2dx"
)

install(
    EXPORT cocos2dx-targets
    DESTINATION share/cocos2dx
    NAMESPACE cocos2dx::
)