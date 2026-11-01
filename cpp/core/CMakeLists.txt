cmake_minimum_required(VERSION 3.28)
project(krkr2core)

include(${CMAKE_SOURCE_DIR}/cmake/KrKr2Modules.cmake)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

add_library(${PROJECT_NAME} INTERFACE)

add_subdirectory(tjs2)
add_subdirectory(log)
add_subdirectory(io)
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
add_subdirectory(visual)
add_subdirectory(utils)

# environ 为组合根：PUBLIC 链齐 foundation + base + domain
target_link_libraries(${PROJECT_NAME} INTERFACE
        core_environ_module
        core_extension_module
)

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