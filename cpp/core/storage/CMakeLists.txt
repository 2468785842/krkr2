cmake_minimum_required(VERSION 3.19)
project(core_storage LANGUAGES CXX)

set(STORAGE_PATH ${CMAKE_CURRENT_LIST_DIR})
set(BASE_PATH ${CMAKE_SOURCE_DIR}/cpp/core/base)
set(UTILS_PATH ${CMAKE_SOURCE_DIR}/cpp/core/utils)

set(STORAGE_PUBLIC_INCLUDES
    ${STORAGE_PATH}
    ${STORAGE_PATH}/impl
    ${KRKR2_CORE_API_INCLUDE_DIRS}
)

# 归档层：TVPOpenArchive / tTVPArchive 基类
set(CORE_STORAGE_ARCHIVE_SOURCES
    ${STORAGE_PATH}/impl/ArchiveBase.cpp
    ${STORAGE_PATH}/impl/ArchiveRegistry.cpp
    ${STORAGE_PATH}/impl/ArchiveFilename.cpp
    ${STORAGE_PATH}/impl/ArchiveStream.cpp
    ${STORAGE_PATH}/impl/StorageGlobals.cpp
    ${STORAGE_PATH}/impl/MemoryStream.cpp
)

add_library(core_storage_archive STATIC ${CORE_STORAGE_ARCHIVE_SOURCES})
target_include_directories(core_storage_archive PUBLIC ${STORAGE_PUBLIC_INCLUDES})
target_link_libraries(core_storage_archive PUBLIC tjs2 core_io core_msg archive_xp3)

# 完整存储：StorageIntf、TVPCreateStream、FileMedia
set(CORE_STORAGE_SOURCE_FILES
    ${BASE_PATH}/StorageIntf.cpp
    ${BASE_PATH}/UtilStreams.cpp
    ${STORAGE_PATH}/impl/CreateStream.cpp
    ${STORAGE_PATH}/impl/FileMedia.cpp
)

add_library(${PROJECT_NAME} STATIC ${CORE_STORAGE_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${STORAGE_PUBLIC_INCLUDES})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${UTILS_PATH}
    ${CMAKE_SOURCE_DIR}/cpp/core/environ
)
target_link_libraries(${PROJECT_NAME} PUBLIC core_storage_archive)

find_package(unrar CONFIG REQUIRED)
find_package(LibArchive REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE unrar::unrar LibArchive::LibArchive)

# 兼容旧目标名
add_library(core_storage_engine ALIAS ${PROJECT_NAME})
