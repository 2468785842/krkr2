cmake_minimum_required(VERSION 3.16)
project(Tests LANGUAGES CXX)

find_package(Catch2 3 REQUIRED)

include(CTest)
include(Catch)

set(TEST_CONFIG_DIR "${CMAKE_CURRENT_BINARY_DIR}")

if(KRKR2_TEST_SCOPE STREQUAL "imageops")
    add_subdirectory(unit-tests/core/visual)
    return()
endif()

add_subdirectory(unit-tests/core/archive)
if(KRKR2_TEST_SCOPE STREQUAL "archives")
    return()
endif()

add_subdirectory(unit-tests/core/movie)
add_subdirectory(unit-tests/core/visual)
add_subdirectory(unit-tests/core/tjs2)

add_subdirectory(unit-tests/plugins)

set(TEST_FILES_PATH ${CMAKE_CURRENT_SOURCE_DIR}/test_files)
configure_file(test_config.h.in test_config.h)
