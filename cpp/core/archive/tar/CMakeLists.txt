cmake_minimum_required(VERSION 3.19)
project(archive_tar LANGUAGES CXX)

set(ARCHIVE_TAR_PATH ${CMAKE_CURRENT_LIST_DIR})
add_library(${PROJECT_NAME} STATIC
    ${ARCHIVE_TAR_PATH}/TARArchive.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE ${KRKR2_CORE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC core_storage)
