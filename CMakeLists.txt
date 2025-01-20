cmake_minimum_required(VERSION 3.19)

set(VCPKG_TARGET_ANDROID ON)

set(ENV{ANDROID_NDK_HOME} "$ENV{ANDROID_NDK}")

include(cmake/vcpkg_android.cmake)
project(krkr2)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(KRKR2CORE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cpp/core)
set(KRKR2PLUGIN_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cpp/plugins)

add_library(${PROJECT_NAME} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/cpp/krkr2_android.cpp)

include(cmake/AndroidNdkModules.cmake)

# build library
add_subdirectory(${KRKR2CORE_PATH})
add_subdirectory(${KRKR2PLUGIN_PATH})

target_include_directories(${PROJECT_NAME} PUBLIC ${KRKR2CORE_PATH}/environ/cocos2d)

target_link_libraries(${PROJECT_NAME} PUBLIC
    -Wl,-Bstatic
    -Wl,--whole-archive
    krkr2plugin krkr2core
    -Wl,--no-whole-archive
)

# cocos2dx
find_package(OpenSSL REQUIRED)
find_package(Freetype REQUIRED)
find_package(WebP CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(TIFF REQUIRED)

find_package(cocos2dx CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC
    cocos2dx::cocos2d cocos2dx::cpp_android_spec
    OpenSSL::SSL OpenSSL::Crypto
    Freetype::Freetype
    WebP::webp WebP::webpdecoder WebP::webpdemux
    ZLIB::ZLIB
    TIFF::TIFF
)

find_package(FFMPEG COMPONENTS
    avutil
    avfilter
    avcodec
    swscale
    swresample REQUIRED
)
target_include_directories(${PROJECT_NAME} PUBLIC ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${FFMPEG_LIBRARIES})

find_package(Boost COMPONENTS locale REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${Boost_LIBRARIES})

find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt-header-only)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog::spdlog_header_only)