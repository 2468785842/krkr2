cmake_minimum_required(VERSION 3.19)
project(core_log LANGUAGES CXX)

set(LOG_PATH ${CMAKE_CURRENT_LIST_DIR})
set(CORE_LOG_SOURCE_FILES
    ${LOG_PATH}/TVPLog.cpp
    ${LOG_PATH}/SpdlogSink.cpp
    ${LOG_PATH}/TVPInitLog.cpp
    ${LOG_PATH}/TVPDialog.cpp
    ${LOG_PATH}/TVPLogLevelConfig.cpp
)

add_library(${PROJECT_NAME} STATIC ${CORE_LOG_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${LOG_PATH})

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC fmt::fmt)
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog)
