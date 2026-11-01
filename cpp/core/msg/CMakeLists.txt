cmake_minimum_required(VERSION 3.19)
project(core_msg LANGUAGES CXX)

set(MSG_PATH ${CMAKE_CURRENT_LIST_DIR})
set(GEN_CODE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)

if(NOT EXISTS ${GEN_CODE_BINARY_DIR})
    file(MAKE_DIRECTORY ${GEN_CODE_BINARY_DIR})
endif()

#===============================================================#
#================== generate message headers ===================#
#===============================================================#
find_package(Python3 REQUIRED)

set(TVP_MESSAGES_DECL_H ${GEN_CODE_BINARY_DIR}/TVPMessages.decl.h)

execute_process(
    TIMEOUT 5000
    COMMAND ${Python3_EXECUTABLE}
        ${MSG_PATH}/script/create_messages.py
        ${TVP_MESSAGES_DECL_H}
    WORKING_DIRECTORY ${MSG_PATH}/script
    COMMAND_ECHO STDOUT
)

add_custom_command(
    OUTPUT ${TVP_MESSAGES_DECL_H}
    COMMAND ${Python3_EXECUTABLE}
        ${MSG_PATH}/script/create_messages.py
        ${TVP_MESSAGES_DECL_H}
    WORKING_DIRECTORY ${MSG_PATH}/script
    DEPENDS
        ${MSG_PATH}/script/messages.json
        ${MSG_PATH}/script/create_messages.py
    COMMENT "Generate TVPMessages.decl.h from messages.json"
)

add_custom_target(GENERATE_MSG_FILES DEPENDS ${TVP_MESSAGES_DECL_H})

set(CORE_MSG_SOURCE_FILES
    ${MSG_PATH}/impl/MessageRegistry.cpp
    ${MSG_PATH}/impl/Error.cpp
    ${MSG_PATH}/impl/About.cpp
    ${MSG_PATH}/impl/Version.cpp
)

add_library(${PROJECT_NAME} STATIC ${CORE_MSG_SOURCE_FILES})
add_dependencies(${PROJECT_NAME} GENERATE_MSG_FILES)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${MSG_PATH}
    $<BUILD_INTERFACE:${GEN_CODE_BINARY_DIR}>
)

target_link_libraries(${PROJECT_NAME} PUBLIC tjs2)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE fmt::fmt)
