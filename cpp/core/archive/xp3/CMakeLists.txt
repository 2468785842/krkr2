cmake_minimum_required(VERSION 3.19)
project(archive_xp3 LANGUAGES CXX)

set(ARCHIVE_XP3_PATH ${CMAKE_CURRENT_LIST_DIR})

add_library(${PROJECT_NAME} STATIC
    ${ARCHIVE_XP3_PATH}/XP3Archive.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${ARCHIVE_XP3_PATH}
    ${KRKR2_CORE_DIR}
)

target_link_libraries(${PROJECT_NAME} PUBLIC tjs2 core_msg)

find_package(ZLIB REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
