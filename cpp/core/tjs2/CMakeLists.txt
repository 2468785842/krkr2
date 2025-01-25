cmake_minimum_required(VERSION 3.19)
project(tjs2)

add_library(${PROJECT_NAME} STATIC)

set(GEN_CODE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)

if(NOT EXISTS ${GEN_CODE_BINARY_DIR})
    file(MAKE_DIRECTORY ${GEN_CODE_BINARY_DIR})
endif()

include("$ENV{VCPKG_ROOT}/scripts/cmake/vcpkg_list.cmake")
include("$ENV{VCPKG_ROOT}/scripts/cmake/vcpkg_find_acquire_program.cmake")

#===============================================================#
#======================= generate parser =======================#
#===============================================================#
vcpkg_find_acquire_program(BISON)

set(BISON_FILES
    ${CMAKE_CURRENT_LIST_DIR}/bison/tjs.y
    ${CMAKE_CURRENT_LIST_DIR}/bison/tjsdate.y
    ${CMAKE_CURRENT_LIST_DIR}/bison/tjspp.y
)

foreach(BISON_FILE ${BISON_FILES})
    get_filename_component(FILE_NAME ${BISON_FILE} NAME_WE)

    execute_process(
        TIMEOUT 5000
        COMMAND ${BISON} ${BISON_FILE}
        WORKING_DIRECTORY ${GEN_CODE_BINARY_DIR}
        COMMAND_ECHO STDOUT
    )

    add_custom_target(generate_bison_${FILE_NAME}_file
        BYPRODUCTS
            ${GEN_CODE_BINARY_DIR}/${FILE_NAME}.tab.cpp
            ${GEN_CODE_BINARY_DIR}/${FILE_NAME}.tab.hpp
        DEPENDS ${BISON_FILE}
        COMMAND ${BISON} ${BISON_FILE}
        WORKING_DIRECTORY ${GEN_CODE_BINARY_DIR}
        COMMENT "Generate Bison File From ${BISON_FILE}"
    )
    add_dependencies(${PROJECT_NAME} generate_bison_${FILE_NAME}_file)
    target_sources(${PROJECT_NAME} PRIVATE ${GEN_CODE_BINARY_DIR}/${FILE_NAME}.tab.cpp)
endforeach()

install(DIRECTORY "${GEN_CODE_BINARY_DIR}/"
    DESTINATION "include/tjs2/parser"
    FILES_MATCHING PATTERN "*.tab.hpp"
)

#===============================================================#
#==================== generate date word =======================#
#===============================================================#
vcpkg_find_acquire_program(PYTHON3)

execute_process(
    TIMEOUT 5000
    COMMAND ${PYTHON3} create_world_map.py ${GEN_CODE_BINARY_DIR}/tjsDateWordMap.inc
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/script
    COMMAND_ECHO STDOUT
)

target_sources(${PROJECT_NAME} PRIVATE
    tjsLex.cpp
    tjsNative.cpp
    tjsRandomGenerator.cpp
    tjsDebug.cpp
    tjsRegExp.cpp
    tjsBinarySerializer.cpp
    tjsInterCodeGen.cpp
    tjsObject.cpp
    tjsConstArrayData.cpp
    tjsUtils.cpp
    tjsException.cpp
    tjsDictionary.cpp
    tjsInterCodeExec.cpp
    tjsVariantString.cpp
    tjsScriptBlock.cpp
    tjsMath.cpp
    tjs.cpp
    tjsMessage.cpp
    tjsDisassemble.cpp
    tjsDate.cpp
    tjsError.cpp
    tjsInterface.cpp
    tjsMT19937ar-cok.cpp
    tjsArray.cpp
    tjsByteCodeLoader.cpp
    tjsDateParser.cpp
    tjsGlobalStringMap.cpp
    tjsOctPack.cpp
    tjsNamespace.cpp
    tjsScriptCache.cpp
    tjsConfig.cpp
    tjsCompileControl.cpp
    tjsObjectExtendable.cpp
    tjsVariant.cpp
    tjsString.cpp
)

install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/"
    DESTINATION "include/tjs2"
    FILES_MATCHING PATTERN "*.h"
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<BUILD_INTERFACE:${GEN_CODE_BINARY_DIR}>
    INTERFACE
        $<INSTALL_INTERFACE:include/tjs2>
        $<INSTALL_INTERFACE:include/tjs2/parser>
)

find_package(Boost COMPONENTS locale REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    Boost::locale
    fmt::fmt-header-only
    spdlog::spdlog_header_only
)