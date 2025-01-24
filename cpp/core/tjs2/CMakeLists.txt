cmake_minimum_required(VERSION 3.19)
project(tjs2)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PRIVATE
    parser/tjs.tab.cpp
    parser/tjspp.tab.cpp
    parser/tjsdate.tab.cpp

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
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/parser>
    INTERFACE
        $<INSTALL_INTERFACE:include/tjs2>
        $<INSTALL_INTERFACE:include/tjs2/parser>
)

target_link_libraries(${PROJECT_NAME} PRIVATE krkr2core)