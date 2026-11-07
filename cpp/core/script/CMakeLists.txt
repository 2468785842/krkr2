cmake_minimum_required(VERSION 3.19)
project(core_script LANGUAGES CXX)

set(SCRIPT_PATH ${CMAKE_CURRENT_LIST_DIR})
set(BASE_PATH ${CMAKE_SOURCE_DIR}/cpp/core/base)
set(CORE_ROOT ${CMAKE_SOURCE_DIR}/cpp/core)

set(CORE_SCRIPT_SOURCE_FILES
    ${BASE_PATH}/ScriptMgnIntf.cpp
)

add_library(${PROJECT_NAME} STATIC ${CORE_SCRIPT_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${KRKR2_CORE_API_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CORE_ROOT}/visual
    ${CORE_ROOT}/visual/impl
    ${CORE_ROOT}/visual/ogl
    ${CORE_ROOT}/sound
    ${CORE_ROOT}/sound/win32
    ${CORE_ROOT}/utils
    ${CORE_ROOT}/utils/impl
    ${CORE_ROOT}/plugin
    ${CORE_ROOT}/extension
    ${CORE_ROOT}/environ
    ${CORE_ROOT}/environ/impl
    ${CORE_ROOT}/environ/ui
    ${CORE_ROOT}/environ/ConfigManager
)
target_link_libraries(${PROJECT_NAME} PUBLIC
    tjs2
    core_msg
    core_sysinit
    core_storage
)
