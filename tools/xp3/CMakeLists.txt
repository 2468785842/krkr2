cmake_minimum_required(VERSION 3.28)
project(xp3 LANGUAGES CXX VERSION 1.0.0)

include(${CMAKE_SOURCE_DIR}/cmake/KrKr2Modules.cmake)

set(XP3_PATH ${CMAKE_CURRENT_LIST_DIR})

add_executable(${PROJECT_NAME}
    ${XP3_PATH}/main.cpp
    ${XP3_PATH}/tool/StdFileBackend.cpp
    ${XP3_PATH}/tool/StorageStubs.cpp
    ${XP3_PATH}/tool/ToolRuntimeStubs.cpp
)

find_package(argparse CONFIG REQUIRED)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${XP3_PATH}/tool
    ${CMAKE_SOURCE_DIR}/cpp/core
    ${CMAKE_SOURCE_DIR}/cpp/core/utils
    ${CMAKE_SOURCE_DIR}/cpp/core/archive/xp3
    ${CMAKE_SOURCE_DIR}/cpp/core/environ
    ${KRKR2_CORE_API_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    argparse::argparse
    core_log
    core_storage
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROGRAM_NAME="${PROJECT_NAME}"
    VERSION="${PROJECT_VERSION}"
)
