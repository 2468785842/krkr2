cmake_minimum_required(VERSION 3.19)
project(core_plugin_module LANGUAGES CXX)

set(PLUGIN_PATH ${CMAKE_CURRENT_LIST_DIR})
set(PLUGIN_SOURCE_FILES
        ${PLUGIN_PATH}/ncbind.cpp
        ${PLUGIN_PATH}/PluginIntf.cpp
        ${PLUGIN_PATH}/PluginImpl.cpp
)

set(PLUGIN_HEADERS_DIR
        ${PLUGIN_PATH}/
)

add_library(${PROJECT_NAME} STATIC ${PLUGIN_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${PLUGIN_HEADERS_DIR})
krkr2_target_link_foundation(${PROJECT_NAME} PUBLIC)
target_link_libraries(${PROJECT_NAME} PRIVATE core_visual_module core_utils_module)
krkr2_target_include_base_headers(${PROJECT_NAME})
krkr2_target_include_environ_headers(${PROJECT_NAME})
