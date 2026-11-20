cmake_minimum_required(VERSION 3.28)
project(core_environ_sdl3 LANGUAGES CXX)

find_package(SDL3 3.4 CONFIG REQUIRED)

add_library(${PROJECT_NAME} STATIC
    SdlAppDelegate.cpp
    SdlKeyCodeConv.cpp
    SdlWindowLayer.cpp
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)
