cmake_minimum_required(VERSION 3.19)
project(core_sysinit LANGUAGES CXX)

set(SYSINIT_PATH ${CMAKE_CURRENT_LIST_DIR})
set(CORE_SYSINIT_SOURCE_FILES
    ${SYSINIT_PATH}/SysInitIntf.cpp
    ${SYSINIT_PATH}/impl/SysInitImpl.cpp
    ${SYSINIT_PATH}/impl/SysInitProtectStub.cpp
)

add_library(${PROJECT_NAME} STATIC ${CORE_SYSINIT_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC
    ${SYSINIT_PATH}
    ${SYSINIT_PATH}/impl
)
krkr2_target_include_base_headers(${PROJECT_NAME})
krkr2_target_include_environ_headers(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${KRKR2_CORE_DIR}/visual
    ${KRKR2_CORE_DIR}/visual/impl
    ${KRKR2_CORE_DIR}/visual/ogl
    ${KRKR2_CORE_DIR}/utils
    ${KRKR2_CORE_DIR}/utils/impl
    ${KRKR2_CORE_DIR}/storage
    ${KRKR2_CORE_DIR}/storage/impl
    ${KRKR2_CORE_DIR}/plugin
)
target_link_libraries(${PROJECT_NAME} PUBLIC tjs2 core_msg)
