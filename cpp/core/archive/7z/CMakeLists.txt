cmake_minimum_required(VERSION 3.19)
project(archive_7z LANGUAGES CXX)

set(ARCHIVE_7Z_PATH ${CMAKE_CURRENT_LIST_DIR})
add_library(${PROJECT_NAME} STATIC
    ${ARCHIVE_7Z_PATH}/7zArchive.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${KRKR2_CORE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC core_storage)

find_package(7zip CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE 7zip::7zip)
