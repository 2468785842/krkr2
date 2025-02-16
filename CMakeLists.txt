cmake_minimum_required(VERSION 3.28)

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

target_include_directories(${PROJECT_NAME} PUBLIC ${KRKR2CORE_PATH}/environ/cocos2d)

# build library
add_subdirectory(${KRKR2CORE_PATH})
add_subdirectory(${KRKR2PLUGIN_PATH})

target_link_libraries(${PROJECT_NAME} PUBLIC
    krkr2plugin krkr2core
)