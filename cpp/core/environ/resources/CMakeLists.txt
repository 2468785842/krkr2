cmake_minimum_required(VERSION 3.19)
project(core_host_assets LANGUAGES CXX)

add_library(${PROJECT_NAME} STATIC LocalAssetReader.cpp HostAssets.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
