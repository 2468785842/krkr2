cmake_minimum_required(VERSION 3.25)
project(unrar LANGUAGES CXX)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "-Wno-logical-op-parentheses -Wno-switch -Wno-dangling-else")

add_library(${PROJECT_NAME})

# Definitions
target_compile_definitions(${PROJECT_NAME} PUBLIC -DUNIX_TIME_NS -DRARDLL -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DRAR_SMP)

# 源文件列表
set(OBJECTS
    rar.cpp strlist.cpp strfn.cpp pathfn.cpp smallfn.cpp global.cpp file.cpp filefn.cpp filcreat.cpp
    archive.cpp arcread.cpp unicode.cpp system.cpp crypt.cpp crc.cpp rawread.cpp encname.cpp
    resource.cpp match.cpp timefn.cpp rdwrfn.cpp consio.cpp options.cpp errhnd.cpp rarvm.cpp secpassword.cpp
    rijndael.cpp getbits.cpp sha1.cpp sha256.cpp blake2s.cpp hash.cpp extinfo.cpp extract.cpp volume.cpp
    list.cpp find.cpp unpack.cpp headers.cpp threadpool.cpp rs16.cpp cmddata.cpp ui.cpp
)

set(LIB_OBJ filestr.cpp scantree.cpp dll.cpp qopen.cpp)

target_sources(${PROJECT_NAME} PRIVATE ${OBJECTS} ${LIB_OBJ})

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT "${PROJECT_NAME}-targets"
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)


file(GLOB HEADERS "${CMAKE_CURRENT_LIST_DIR}/*.hpp")
install(FILES ${HEADERS} DESTINATION "include/${PROJECT_NAME}")

include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
  INSTALL_DESTINATION "share/${PROJECT_NAME}"
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake"
  DESTINATION "share/${PROJECT_NAME}"
)

install(
    EXPORT "${PROJECT_NAME}-targets"
    DESTINATION share/${PROJECT_NAME}
    NAMESPACE ${PROJECT_NAME}::
)
