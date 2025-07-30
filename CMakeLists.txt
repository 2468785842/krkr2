cmake_minimum_required(VERSION 3.28)
set(CMAKE_CXX_STANDARD 17)          # 全局要求 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON) # 强制
set(CMAKE_CXX_EXTENSIONS OFF)       # 关闭编译器扩展
find_program(CCACHE_PROGRAM ccache)
if (CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Ccache found: ${CCACHE_PROGRAM}")
else ()
    message(STATUS "Ccache not found")
endif ()

if(ANDROID)
    set(VCPKG_TARGET_ANDROID ON)
    set(ENV{ANDROID_NDK_HOME} "$ENV{ANDROID_NDK}")
    add_compile_options(-Wno-inconsistent-missing-override)
    include(cmake/vcpkg_android.cmake)
else()
    set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
endif()

set(APP_NAME krkr2)

project(${APP_NAME})

set(CMAKE_BUILD_PARALLEL_LEVEL 8)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(KRKR2CORE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cpp/core)
set(KRKR2PLUGIN_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cpp/plugins)
if(MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/EHsc>")
endif()
if(MSVC)
    add_link_options("/ignore:4099")
    # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
if(ANDROID)
    add_library(${PROJECT_NAME} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/android/cpp/krkr2_android.cpp)
elseif(LINUX)
    add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/linux/main.cpp)
elseif(WINDOWS)
    add_executable(${PROJECT_NAME} 
        ${CMAKE_CURRENT_SOURCE_DIR}/windows/main.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/windows/game.rc
    )
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${KRKR2CORE_PATH}/environ/cocos2d)

# build library
add_subdirectory(${KRKR2CORE_PATH})
add_subdirectory(${KRKR2PLUGIN_PATH})

target_link_libraries(${PROJECT_NAME} PUBLIC
    krkr2plugin krkr2core
)

if(NOT ANDROID)
    include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CocosBuildHelpers.cmake")

    set(GAME_RES_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/ui/cocos-studio")

    if(WINDOWS)
        cocos_mark_multi_resources(common_res_files RES_TO "Resources" FOLDERS ${GAME_RES_FOLDER})
    endif()

    # mark app resources
    setup_cocos_app_config(${APP_NAME})

    if(WINDOWS)
        target_sources(${PROJECT_NAME} PRIVATE ${common_res_files})
        cocos_copy_target_dll(${APP_NAME})
    endif()

    if(LINUX OR WINDOWS)
        cocos_get_resource_path(APP_RES_DIR ${APP_NAME})
        cocos_copy_target_res(${APP_NAME} LINK_TO ${APP_RES_DIR} FOLDERS ${GAME_RES_FOLDER})
    endif()
endif()

if(MSVC)
  target_compile_options(${PROJECT_NAME} PUBLIC "/ZI")
  target_link_options(${PROJECT_NAME} PUBLIC "/INCREMENTAL")
endif()

if(NOT DISABLE_TEST AND NOT ANDROID)
    enable_testing()
    add_subdirectory(tests)
endif ()