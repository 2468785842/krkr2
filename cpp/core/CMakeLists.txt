cmake_minimum_required(VERSION 3.28)
project(krkr2core)

add_library(${PROJECT_NAME} INTERFACE)

add_subdirectory(tjs2)
add_subdirectory(base)
add_subdirectory(environ)
add_subdirectory(extension)
add_subdirectory(plugin)
add_subdirectory(movie)
add_subdirectory(sound)
add_subdirectory(visual)
add_subdirectory(utils)

target_link_libraries(${PROJECT_NAME} INTERFACE
        tjs2
        core_base_module
        core_environ_module
        core_extension_module
        core_plugin_module
        core_movie_module
        core_sound_module
        core_visual_module
        core_utils_module
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

# oboe
if(ANDROID)
    find_package (oboe CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} INTERFACE oboe::oboe)
endif()

# cocos2dx
find_package(cocos2dx CONFIG REQUIRED)

## Link libraries
target_link_libraries(${PROJECT_NAME} INTERFACE
    # cocos2d
    cocos2dx::cocos2d
)