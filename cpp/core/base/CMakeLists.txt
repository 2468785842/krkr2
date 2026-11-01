cmake_minimum_required(VERSION 3.19)
project(core_base_module LANGUAGES CXX)

set(BASE_PATH ${CMAKE_CURRENT_LIST_DIR})
set(CORE_ROOT ${CMAKE_SOURCE_DIR}/cpp/core)

set(BASE_SOURCE_FILES
    ${BASE_PATH}/BinaryStream.cpp
    ${BASE_PATH}/CharacterSet.cpp
    ${BASE_PATH}/SystemIntf.cpp
    ${BASE_PATH}/TextStream.cpp

    ${BASE_PATH}/impl/FileSelector.cpp
    ${BASE_PATH}/impl/NativeEventQueue.cpp
    ${BASE_PATH}/impl/StorageImpl.cpp
    ${BASE_PATH}/impl/LocalFileBackend.cpp
    ${BASE_PATH}/impl/FileBackendInit.cpp
    ${BASE_PATH}/impl/EngineArchiveInit.cpp
    ${BASE_PATH}/impl/SystemImpl.cpp
)

add_library(${PROJECT_NAME} STATIC ${BASE_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${KRKR2_CORE_API_INCLUDE_DIRS})
# 域模块仅头文件依赖；链接在 krkr2core / environ 组合根完成（Phase 4 去环）
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
    ${CORE_ROOT}/storage
    ${CORE_ROOT}/storage/impl
    ${CORE_ROOT}/archive/xp3
)
target_link_libraries(${PROJECT_NAME} PUBLIC
    tjs2
    core_msg
    core_event
    core_kag
    core_sysinit
    core_script
    archive_xp3
    archive_zip
    archive_7z
    archive_tar
    core_io
    core_storage_engine
)

find_package(uchardet CONFIG REQUIRED)
find_package(unrar CONFIG REQUIRED)
find_package(LibXml2 REQUIRED)
include_directories(${LIBXML2_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBXML2_LIBRARIES})
find_package(LibArchive REQUIRED)
find_package(zstd CONFIG REQUIRED)
find_package(cocos2dx CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    uchardet::libuchardet
    unrar::unrar
    LibArchive::LibArchive
    zstd::libzstd

    cocos2dx::cocos2d
)
