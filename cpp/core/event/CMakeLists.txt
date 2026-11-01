cmake_minimum_required(VERSION 3.19)
project(core_event LANGUAGES CXX)

set(EVENT_PATH ${CMAKE_CURRENT_LIST_DIR})
set(CORE_EVENT_SOURCE_FILES
    ${EVENT_PATH}/EventIntf.cpp
    ${EVENT_PATH}/impl/EventImpl.cpp
)

add_library(${PROJECT_NAME} STATIC ${CORE_EVENT_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC
    ${EVENT_PATH}
    ${EVENT_PATH}/impl
)
target_link_libraries(${PROJECT_NAME} PUBLIC
    tjs2
    core_io
    core_msg
    core_sysinit
    core_script
)
krkr2_target_include_base_headers(${PROJECT_NAME})
krkr2_target_include_environ_headers(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${KRKR2_CORE_DIR}/visual
    ${KRKR2_CORE_DIR}/visual/impl
    ${KRKR2_CORE_DIR}/utils
    ${KRKR2_CORE_DIR}/utils/impl
)
