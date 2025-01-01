cmake_minimum_required(VERSION 3.19)

set(VCPKG_CMAKE_SYSTEM_NAME "Android")

set(VCPKG_TARGET_ANDROID ON)

set(ENV{ANDROID_NDK_HOME} "$ENV{ANDROID_NDK}")

include(cmake/vcpkg_android.cmake)
project(krkr2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(KRKR2CORE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cpp/core)
set(KRKR2PLUGIN_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cpp/plugins)

add_library(${PROJECT_NAME} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/cpp/krkr2_android.cpp)

find_package(unofficial-breakpad CONFIG REQUIRED)

find_package(COCOS2DX REQUIRED)

set(COCOS2DX_PATH "${COCOS2DX_SOURCE_DIR}")

include_directories( 
    ${COCOS2DX_PATH}
    ${COCOS2DX_PATH}/cocos
    ${COCOS2DX_PATH}/cocos/scripting
    ${COCOS2DX_PATH}/cocos/editor-support
    ${COCOS2DX_PATH}/extensions
)

set(COCOS2DX_EXTERNAL_HEADER_DIRS
    ${COCOS2DX_PATH}/external
    ${COCOS2DX_PATH}/external/png/include/android
    ${COCOS2DX_PATH}/external/webp/include/android
    ${COCOS2DX_PATH}/external/freetype2/include/android
    ${COCOS2DX_PATH}/external/freetype2/include/android/freetype2

    ${COCOS2DX_PATH}/external/zlib/prebuilt/android/${ANDROID_ABI}
    ${COCOS2DX_PATH}/external/tiff/prebuilt/android/${ANDROID_ABI}
    ${COCOS2DX_PATH}/external/chipmunk/prebuilt/android/${ANDROID_ABI}
    ${COCOS2DX_PATH}/external/bullet/prebuilt/android/${ANDROID_ABI}
)

find_package(FFMPEG COMPONENTS 
    avutil
    avfilter
    avcodec
    swscale
    swresample REQUIRED
)

include(cmake/AndroidNdkModules.cmake)

# build library
add_subdirectory(${KRKR2CORE_PATH})
add_subdirectory(${KRKR2PLUGIN_PATH})

add_subdirectory("${COCOS2DX_SOURCE_DIR}/cocos/platform/android" "${CMAKE_BINARY_DIR}/cocos2dx/android")

target_include_directories(${PROJECT_NAME} PUBLIC
    ${KRKR2CORE_PATH}/environ/cocos2d
    ${COCOS2DX_PATH}/cocos
    ${COCOS2DX_PATH}/cocos/audio/include
)

target_link_libraries(${PROJECT_NAME} PUBLIC unofficial::breakpad::libbreakpad_client)

target_link_libraries(${PROJECT_NAME} PUBLIC
    -Wl,-Bstatic
    -Wl,--whole-archive # cpp_android_spec for jni function, this is important to add whole libraries !
    cpp_android_spec krkr2plugin krkr2core
    -Wl,--no-whole-archive
)