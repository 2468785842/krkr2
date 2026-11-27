cmake_minimum_required(VERSION 3.28)
project(tjs2)

add_library(${PROJECT_NAME} STATIC)
target_compile_definitions(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:_DEBUG>)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    # TJS represents empty strings with null pointers, including inline callers.
    target_compile_options(${PROJECT_NAME} PUBLIC -fno-delete-null-pointer-checks)
endif()

set(GEN_CODE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/gen)

if(WINDOWS)
    target_compile_options(${PROJECT_NAME} PRIVATE "/FS")
endif()

if(NOT EXISTS ${GEN_CODE_BINARY_DIR})
    file(MAKE_DIRECTORY ${GEN_CODE_BINARY_DIR})
endif()

#===============================================================#
#======================= generate parser =======================#
#===============================================================#
find_program(BISON_EXECUTABLE NAMES win_bison.exe bison.exe bison REQUIRED)

set(BISON_FILES
    ${CMAKE_CURRENT_LIST_DIR}/bison/tjs.y
    ${CMAKE_CURRENT_LIST_DIR}/bison/tjsdate.y
    ${CMAKE_CURRENT_LIST_DIR}/bison/tjspp.y
)
set(PARSE_TAB_CPP_FILES)
foreach(BISON_FILE ${BISON_FILES})
    get_filename_component(FILE_NAME ${BISON_FILE} NAME_WE)
    set(BISON_COMMAND "${BISON_EXECUTABLE}")
    if(CMAKE_HOST_WIN32)
        # WinFlexBison 2.5.25 uses non-atomic _tempnam/fopen for m4 files.
        # Isolate concurrent parsers without serializing the whole build.
        set(BISON_TEMP_DIR "${GEN_CODE_BINARY_DIR}/tmp/${FILE_NAME}")
        file(MAKE_DIRECTORY "${BISON_TEMP_DIR}")
        set(BISON_COMMAND "${CMAKE_COMMAND}" -E env
            "TMP=${BISON_TEMP_DIR}" "TEMP=${BISON_TEMP_DIR}" "${BISON_EXECUTABLE}")
    endif()

    add_custom_command(
        OUTPUT
            ${GEN_CODE_BINARY_DIR}/${FILE_NAME}.tab.cpp
            ${GEN_CODE_BINARY_DIR}/${FILE_NAME}.tab.hpp
        DEPENDS ${BISON_FILE}
        COMMAND ${BISON_COMMAND} ${BISON_FILE}
        WORKING_DIRECTORY ${GEN_CODE_BINARY_DIR}
        COMMENT "Generate Bison File From ${BISON_FILE}"
        VERBATIM
    )

    list(APPEND PARSE_TAB_CPP_FILES ${GEN_CODE_BINARY_DIR}/${FILE_NAME}.tab.cpp)
endforeach()

add_custom_target(GENERATE_CPP_FILES DEPENDS ${PARSE_TAB_CPP_FILES})
add_dependencies(${PROJECT_NAME} GENERATE_CPP_FILES)

install(DIRECTORY "${GEN_CODE_BINARY_DIR}/"
    DESTINATION "include/tjs2/parser"
    FILES_MATCHING PATTERN "*.tab.hpp"
)

#===============================================================#
#==================== generate date word =======================#
#===============================================================#
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_custom_command(
    OUTPUT ${GEN_CODE_BINARY_DIR}/tjsDateWordMap.inc
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/script/create_world_map.py
        ${GEN_CODE_BINARY_DIR}/tjsDateWordMap.inc
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/script/create_world_map.py
        ${CMAKE_CURRENT_LIST_DIR}/script/dp_word_table.txt
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/script
    VERBATIM
)
add_custom_target(GENERATE_TJS_DATE_WORDS DEPENDS ${GEN_CODE_BINARY_DIR}/tjsDateWordMap.inc)
add_dependencies(${PROJECT_NAME} GENERATE_TJS_DATE_WORDS)

target_sources(${PROJECT_NAME} PRIVATE
    ${PARSE_TAB_CPP_FILES}
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

find_package(Boost COMPONENTS locale REQUIRED NO_MODULE)
find_package(fmt CONFIG REQUIRED)
find_package(oniguruma CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PUBLIC
    core_log
    Boost::locale
    fmt::fmt
    oniguruma::onig
)

if(KRKR2_SCRIPT_BACKEND STREQUAL "ciallang")
    # Ciallang is built from the git submodule in external/Ciallang (see
    # .gitmodules). The vcpkg port in vcpkg/ports/ciallang pins the same upstream
    # for external consumers; this build path keeps the runtime editable, so a
    # compatibility fix is a submodule commit rather than a port patch.
    if(NOT TARGET Ciallang)
        set(CIAL_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(CIAL_BUILD_CLI OFF CACHE BOOL "" FORCE)
        set(CIAL_BUILD_FORMATTER OFF CACHE BOOL "" FORCE)
        set(CIAL_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
        set(CIAL_BUILD_FUZZ OFF CACHE BOOL "" FORCE)
        set(CIAL_BUILD_BENCH OFF CACHE BOOL "" FORCE)
        set(CIAL_ENABLE_JIT OFF CACHE BOOL "" FORCE)
        set(CIAL_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
        set(CIAL_INSTALL_SDK OFF CACHE BOOL "" FORCE)
        add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../../external/Ciallang"
                         "${CMAKE_BINARY_DIR}/ciallang-src" EXCLUDE_FROM_ALL)
    endif()
    add_library(core_ciallang_bridge OBJECT CiallangRuntime.cpp)
    target_compile_features(core_ciallang_bridge PRIVATE cxx_std_20)
    target_include_directories(core_ciallang_bridge PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${GEN_CODE_BINARY_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/../common
    )
    target_link_libraries(core_ciallang_bridge PRIVATE
        Ciallang::Ciallang
        core_log
        Boost::locale
    )
    if(MSVC)
        target_compile_options(core_ciallang_bridge PRIVATE /EHsc /utf-8)
    endif()
    target_compile_definitions(tjs2 PRIVATE KRKR2_ENABLE_CIALLANG=1)
    target_link_libraries(tjs2 PRIVATE core_ciallang_bridge)
endif()
