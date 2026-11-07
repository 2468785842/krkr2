cmake_minimum_required(VERSION 3.28)
project(krkr2core)

include(${CMAKE_SOURCE_DIR}/cmake/KrKr2Modules.cmake)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

add_library(${PROJECT_NAME} INTERFACE)

add_subdirectory(tjs2)
add_subdirectory(log)
add_subdirectory(io)
add_subdirectory(input)
add_subdirectory(msg)
add_subdirectory(sysinit)
add_subdirectory(script)
add_subdirectory(event)
add_subdirectory(kag)
add_subdirectory(archive)
add_subdirectory(storage)
add_subdirectory(base)
add_subdirectory(environ)
add_subdirectory(extension)
add_subdirectory(plugin)
add_subdirectory(movie)
add_subdirectory(sound)
add_subdirectory(imageops)
add_subdirectory(visual)
add_subdirectory(utils)

# environ 为组合根：PUBLIC 链齐 foundation + base + domain
# 核心 domain 静态库互相引用（依赖检查容许的既有 domain 环）。GNU ld 对
# 静态归档只做单遍扫描，互相拉取的成员会在最终链接留下未解析符号
# （base 的 ScriptMgnIntf 由更靠后的 core_script 拉出时，utils 已扫过）。
# RESCAN 链接组等价 --start-group/--end-group，让 ld 重复扫描直到收敛；
# MSVC 与 Apple 链接器不支持该特性，保持原有顺序路径。
set(KRKR2_CORE_ARCHIVE_TARGETS
    core_environ_module
    core_base_module
    core_extension_module
    core_plugin_module
    core_movie_module
    core_visual_module
    core_sound_module
    core_imageops
    core_utils_module
    core_event
    core_script
    core_sysinit
    core_storage
    core_storage_archive
    archive_xp3
    archive_zip
    archive_7z
    archive_tar
    core_io
    core_msg
    core_kag
    core_input
    tjs2
    core_log
)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    target_link_libraries(${PROJECT_NAME} INTERFACE
        "$<LINK_GROUP:RESCAN,${KRKR2_CORE_ARCHIVE_TARGETS}>"
    )
else()
    target_link_libraries(${PROJECT_NAME} INTERFACE
        core_environ_module
        core_extension_module
    )
endif()

target_include_directories(${PROJECT_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(${PROJECT_NAME} INTERFACE
    -DTJS_TEXT_OUT_CRLF
    -D__STDC_CONSTANT_MACROS
    -DUSE_UNICODE_FSTRING
)

if(NOT APPLE)
    # 启用 OpenMP 支持
    find_package(OpenMP REQUIRED)

    target_link_libraries(${PROJECT_NAME} INTERFACE OpenMP::OpenMP_CXX)
    target_compile_options(${PROJECT_NAME} INTERFACE ${OpenMP_CXX_FLAGS})
endif ()

if(ANDROID)
    target_link_libraries(${PROJECT_NAME} INTERFACE
        log
        android
        EGL
        GLESv2
        GLESv1_CM
        OpenSLES
    )
endif()

# cocos2dx
find_package(cocos2dx CONFIG REQUIRED)

## Link libraries
target_link_libraries(${PROJECT_NAME} INTERFACE
    # cocos2d
    cocos2dx::cocos2d
    $<$<BOOL:${ANDROID}>:$<LINK_LIBRARY:WHOLE_ARCHIVE,cocos2dx::cpp_android_spec>>
)
