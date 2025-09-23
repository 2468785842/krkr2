cmake_minimum_required(VERSION 3.19)
project(core_base_module LANGUAGES CXX)

set(BASE_PATH ${CMAKE_CURRENT_LIST_DIR})
set(BASE_SOURCE_FILES
    ${BASE_PATH}/7zArchive.cpp
    ${BASE_PATH}/BinaryStream.cpp
    ${BASE_PATH}/CharacterSet.cpp
    ${BASE_PATH}/EventIntf.cpp
    ${BASE_PATH}/ScriptMgnIntf.cpp
    ${BASE_PATH}/StorageIntf.cpp
    ${BASE_PATH}/SysInitIntf.cpp
    ${BASE_PATH}/SystemIntf.cpp
    ${BASE_PATH}/TARArchive.cpp
    ${BASE_PATH}/TextStream.cpp
    ${BASE_PATH}/UtilStreams.cpp
    ${BASE_PATH}/XP3Archive.cpp
    ${BASE_PATH}/ZIPArchive.cpp
    ${BASE_PATH}/KAGParser.cpp
    ${BASE_PATH}/MsgIntf.cpp

    ${BASE_PATH}/impl/EventImpl.cpp
    ${BASE_PATH}/impl/FileSelector.cpp
    ${BASE_PATH}/impl/NativeEventQueue.cpp
    ${BASE_PATH}/impl/ScriptMgnImpl.cpp
    ${BASE_PATH}/impl/StorageImpl.cpp
    ${BASE_PATH}/impl/SysInitImpl.cpp
    ${BASE_PATH}/impl/SystemImpl.cpp
    ${BASE_PATH}/impl/MsgImpl.cpp
)

set(BASE_HEADERS_DIR
        ${BASE_PATH}/
        ${BASE_PATH}/impl
)

add_library(${PROJECT_NAME} STATIC ${BASE_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${BASE_HEADERS_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC tjs2 PRIVATE core_visual_module core_plugin_module core_environ_module core_extension_module core_sound_module core_utils_module)

find_package(uchardet CONFIG REQUIRED)
find_package(unrar CONFIG REQUIRED)
find_package(LibArchive REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(cocos2dx CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    uchardet::libuchardet
    unrar::unrar
    LibArchive::LibArchive
    zstd::libzstd

    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    cocos2dx::cocos2d
)
