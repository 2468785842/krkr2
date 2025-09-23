cmake_minimum_required(VERSION 3.19)
project(core_extension_module LANGUAGES CXX)

set(EXTENSION_PATH ${CMAKE_CURRENT_LIST_DIR})
set(EXTENSION_SOURCE_FILES
    ${EXTENSION_PATH}/Extension.cpp
)

set(EXTENSION_HEADERS_DIR
    ${EXTENSION_PATH}/
)

add_library(${PROJECT_NAME} STATIC ${EXTENSION_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${EXTENSION_HEADERS_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC tjs2)
