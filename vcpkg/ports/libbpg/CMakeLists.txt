cmake_minimum_required(VERSION 3.15)
project(libbpg)

# 设置选项
option(USE_X265 "Enable x265 for the encoder" ON)
option(USE_JCTVC "Enable JCTVC code for the encoder" OFF)

# 编译选项
set(CMAKE_C_FLAGS "-fno-asynchronous-unwind-tables -fdata-sections -ffunction-sections -fno-math-errno -fno-signed-zeros -fno-tree-vectorize -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")

add_library(libbpg)

target_compile_definitions(${PROJECT_NAME} PUBLIC -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_REENTRANT)
target_compile_definitions(${PROJECT_NAME} PRIVATE -DHAVE_AV_CONFIG_H)

# JCTVC 高位深度支持
if(USE_JCTVC)
    add_definitions(-DRExt__HIGH_BIT_DEPTH_SUPPORT)
endif()

if(USE_X265)
    # x265 配置
    target_compile_definitions(${PROJECT_NAME} PUBLIC -DUSE_X265)
endif()

# 源文件
set(LIBBPG_SRC
    libavcodec/hevc_cabac.c libavcodec/hevc_filter.c libavcodec/hevc.c
    libavcodec/hevcpred.c libavcodec/hevc_refs.c libavcodec/hevcdsp.c
    libavcodec/hevc_mvs.c libavcodec/hevc_ps.c libavcodec/hevc_sei.c
    libavcodec/utils.c libavcodec/cabac.c libavcodec/golomb.c libavcodec/videodsp.c
    libavutil/mem.c libavutil/buffer.c libavutil/log2_tab.c libavutil/frame.c
    libavutil/pixdesc.c libavutil/md5.c libbpg.c
)
# 定义目标
target_sources(${PROJECT_NAME} PRIVATE ${LIBBPG_SRC})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<INSTALL_INTERFACE:include>
)

install(
    TARGETS ${PROJECT_NAME}
    EXPORT "${PROJECT_NAME}-targets"
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES "${CMAKE_CURRENT_LIST_DIR}/libbpg.h" DESTINATION "include")

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
