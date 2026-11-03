cmake_minimum_required(VERSION 3.28)

find_program(CCACHE_PROGRAM ccache)

if (CCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Ccache found: ${CCACHE_PROGRAM}")
endif ()

add_compile_definitions(
        $<$<CONFIG:Debug>:DEBUG>
        $<$<CONFIG:Debug>:_DEBUG>   # 对于 MSVC 兼容性
        $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
)

option(KRKR2_USE_COCOS_UI "Build the optional Cocos launcher and preference pages" ON)
if(NOT KRKR2_USE_COCOS_UI AND (ANDROID OR IOS))
    message(FATAL_ERROR "KRKR2_USE_COCOS_UI=OFF currently requires a desktop CLI entry point")
endif()

option(KRKR2_ENABLE_UI_TEST_DRIVER "Build the opt-in desktop UI contract driver" OFF)
if(KRKR2_ENABLE_UI_TEST_DRIVER AND (ANDROID OR IOS))
    message(FATAL_ERROR "The UI contract driver is desktop-only")
endif()

option(KRKR2_BUILD_APP "Build the game runtime and bundled plugins" ON)
option(KRKR2_BUILD_SDL3_PROTOTYPE "Build the separate SDL3/TJS desktop shell experiment" OFF)
if(KRKR2_BUILD_SDL3_PROTOTYPE AND (ANDROID OR IOS))
    message(FATAL_ERROR "The SDL3 prototype is desktop-only")
endif()
option(ENABLE_TESTS "Build engine tests (desktop only)" ON)
option(BUILD_TOOLS "Build command-line tools (desktop only)" ON)
if(KRKR2_ENABLE_UI_TEST_DRIVER AND (NOT KRKR2_BUILD_APP OR NOT ENABLE_TESTS))
    message(FATAL_ERROR "The UI contract driver requires KRKR2_BUILD_APP=ON and ENABLE_TESTS=ON")
endif()
set(KRKR2_TEST_SCOPE "full" CACHE STRING "Test scope: full, strings, archives, imageops, launch or platform")
set_property(CACHE KRKR2_TEST_SCOPE PROPERTY STRINGS full strings archives imageops launch platform)
if(NOT KRKR2_TEST_SCOPE MATCHES "^(full|strings|archives|imageops|launch|platform)$")
    message(FATAL_ERROR "KRKR2_TEST_SCOPE must be full, strings, archives, imageops, launch or platform")
endif()
option(KRKR2_ENABLE_PSD "Build the PSD plugin" ON)
option(KRKR2_ENABLE_CRASH_UPLOAD "Build optional crash report upload" OFF)

# vcpkg installs dependencies during project(), before targets are configured.
set(VCPKG_MANIFEST_NO_DEFAULT_FEATURES ON)
if(KRKR2_BUILD_APP OR KRKR2_BUILD_SDL3_PROTOTYPE OR BUILD_TOOLS OR (ENABLE_TESTS AND NOT KRKR2_TEST_SCOPE MATCHES "^(imageops|launch|platform)$"))
    list(APPEND VCPKG_MANIFEST_FEATURES foundation)
endif()
if(KRKR2_BUILD_APP OR (ENABLE_TESTS AND KRKR2_TEST_SCOPE STREQUAL "full"))
    list(APPEND VCPKG_MANIFEST_FEATURES engine)
    if(KRKR2_ENABLE_PSD)
        list(APPEND VCPKG_MANIFEST_FEATURES psd)
    endif()
    if(KRKR2_ENABLE_CRASH_UPLOAD)
        list(APPEND VCPKG_MANIFEST_FEATURES crash-upload)
    endif()
endif()
if(ENABLE_TESTS AND NOT (ANDROID OR IOS))
    list(APPEND VCPKG_MANIFEST_FEATURES tests)
    if(KRKR2_TEST_SCOPE STREQUAL "archives")
        list(APPEND VCPKG_MANIFEST_FEATURES archives)
    elseif(KRKR2_TEST_SCOPE STREQUAL "imageops")
        list(APPEND VCPKG_MANIFEST_FEATURES imageops)
    endif()
endif()
if(BUILD_TOOLS AND NOT (ANDROID OR IOS))
    list(APPEND VCPKG_MANIFEST_FEATURES tools)
endif()
list(REMOVE_DUPLICATES VCPKG_MANIFEST_FEATURES)
if(KRKR2_BUILD_SDL3_PROTOTYPE)
    list(APPEND VCPKG_MANIFEST_FEATURES sdl3-prototype)
endif()

if(ANDROID)
    set(VCPKG_TARGET_ANDROID ON)
    set(ENV{ANDROID_NDK_HOME} "$ENV{ANDROID_NDK}")
    add_compile_options(-Wno-inconsistent-missing-override)
    include(cmake/vcpkg_android.cmake)
elseif(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
    set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
endif()

set(APP_NAME krkr2)

project(${APP_NAME})

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(KRKR2CORE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cpp/core)
set(KRKR2PLUGIN_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cpp/plugins)

if(MSVC)
    add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/EHsc>" "/MP" "/utf-8")
    add_link_options("/ignore:4099" "/INCREMENTAL" "/DEBUG:FASTLINK")
    # set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# A tools-only configuration never enters the engine dependency graph.
if(NOT KRKR2_BUILD_APP AND (NOT ENABLE_TESTS OR NOT KRKR2_TEST_SCOPE STREQUAL "full"))
    if((NOT BUILD_TOOLS AND NOT ENABLE_TESTS AND NOT KRKR2_BUILD_SDL3_PROTOTYPE) OR ANDROID OR IOS)
        message(FATAL_ERROR "No supported build targets selected")
    endif()
    include(cmake/KrKr2Modules.cmake)
    include_directories(${KRKR2CORE_PATH}/common)
    add_compile_definitions(TJS_TEXT_OUT_CRLF __STDC_CONSTANT_MACROS USE_UNICODE_FSTRING)
    if(KRKR2_BUILD_SDL3_PROTOTYPE OR BUILD_TOOLS OR NOT KRKR2_TEST_SCOPE MATCHES "^(imageops|launch|platform)$")
        add_subdirectory(cpp/core/log)
        add_subdirectory(cpp/core/tjs2)
    endif()
    if(KRKR2_BUILD_SDL3_PROTOTYPE OR (ENABLE_TESTS AND KRKR2_TEST_SCOPE STREQUAL "platform"))
        add_subdirectory(cpp/core/input)
    endif()
    if(KRKR2_BUILD_SDL3_PROTOTYPE)
        add_subdirectory(tools/sdl3-prototype)
    endif()
    if(ENABLE_TESTS AND KRKR2_TEST_SCOPE STREQUAL "launch")
        add_subdirectory(cpp/core/environ/launch)
    endif()
    if(ENABLE_TESTS AND KRKR2_TEST_SCOPE STREQUAL "imageops")
        add_subdirectory(cpp/core/imageops)
    endif()
    if(BUILD_TOOLS OR (ENABLE_TESTS AND KRKR2_TEST_SCOPE STREQUAL "archives"))
        add_subdirectory(cpp/core/msg)
        add_subdirectory(cpp/core/io)
        if(BUILD_TOOLS)
            add_subdirectory(cpp/core/archive/xp3)
        endif()
        if(ENABLE_TESTS AND KRKR2_TEST_SCOPE STREQUAL "archives")
            add_subdirectory(cpp/core/archive/zip)
            add_subdirectory(cpp/core/archive/tar)
        endif()
        add_subdirectory(cpp/core/storage)
    endif()
    if(BUILD_TOOLS)
        add_subdirectory(tools)
    endif()
    if(ENABLE_TESTS)
        enable_testing()
        find_package(Catch2 3 REQUIRED)
        include(Catch)
        set(TEST_CONFIG_DIR ${CMAKE_CURRENT_BINARY_DIR})
        if(KRKR2_TEST_SCOPE STREQUAL "platform")
            add_subdirectory(tests/unit-tests/core/environ)
        elseif(KRKR2_TEST_SCOPE STREQUAL "archives")
            add_subdirectory(tests/unit-tests/core/archive)
        elseif(KRKR2_TEST_SCOPE STREQUAL "launch")
            add_subdirectory(tests/unit-tests/core/launch)
        elseif(KRKR2_TEST_SCOPE STREQUAL "imageops")
            add_subdirectory(tests/unit-tests/core/visual)
        else()
            add_subdirectory(tests/unit-tests/core/tjs2)
        endif()
    endif()
    return()
endif()

if(KRKR2_BUILD_APP)
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED ${CMAKE_CURRENT_SOURCE_DIR}/platforms/android/cpp/krkr2_android.cpp)
        find_package(unofficial-breakpad CONFIG REQUIRED)
        target_link_libraries(${PROJECT_NAME} PUBLIC unofficial::breakpad::libbreakpad_client)
    elseif(LINUX)
        add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux/main.cpp)
    elseif(WINDOWS)
        add_executable(${PROJECT_NAME}
            ${CMAKE_CURRENT_SOURCE_DIR}/platforms/windows/main.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/platforms/windows/game.rc
        )
    elseif(IOS)
    #        list(APPEND GAME_HEADER
    #                proj.ios_mac/ios/AppController.h
    #                proj.ios_mac/ios/RootViewController.h
    #        )
    #        set(APP_UI_RES
    #                proj.ios_mac/ios/LaunchScreen.storyboard
    #                proj.ios_mac/ios/LaunchScreenBackground.png
    #                proj.ios_mac/ios/Images.xcassets
    #        )
    #        list(APPEND GAME_SOURCE
    #                proj.ios_mac/ios/main.m
    #                proj.ios_mac/ios/AppController.mm
    #                proj.ios_mac/ios/RootViewController.mm
    #                proj.ios_mac/ios/Prefix.pch
    #                ${APP_UI_RES}
    #        )
    elseif(MACOS)
        set(APP_UI_RES
                ${CMAKE_CURRENT_SOURCE_DIR}/platforms/apple/macos/Icon.icns
                ${CMAKE_CURRENT_SOURCE_DIR}/platforms/apple/macos/Info.plist
        )
        add_executable(${PROJECT_NAME}
                ${CMAKE_CURRENT_SOURCE_DIR}/platforms/apple/macos/main.cpp
                ${CMAKE_CURRENT_SOURCE_DIR}/platforms/apple/macos/Prefix.pch
                ${APP_UI_RES}
        )
    endif()

    target_include_directories(${PROJECT_NAME} PUBLIC ${KRKR2CORE_PATH}/environ/cocos2d)

endif()

# external lib
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cpp/external)

# build library
add_subdirectory(${KRKR2CORE_PATH})
add_subdirectory(${KRKR2PLUGIN_PATH})

if(KRKR2_BUILD_APP)
    target_link_libraries(${PROJECT_NAME} PUBLIC
        krkr2plugin krkr2core
    )

    ## resources
    if(NOT ANDROID)
        include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/CocosBuildHelpers.cmake")

        set(GAME_RES_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/assets")

        include(cmake/RuntimeAssets.cmake)
        if(WINDOWS OR APPLE)
            cocos_mark_resources(FILES ${common_res_files} BASEDIR ${GAME_RES_FOLDER} RESOURCEBASE "Resources")
        endif()

        # mark app resources
        setup_cocos_app_config(${APP_NAME})

        if(APPLE)
            set_target_properties(${APP_NAME} PROPERTIES RESOURCE "${APP_UI_RES}")
            target_sources(${APP_NAME} PRIVATE ${common_res_files})
            if(MACOSX)
                set_target_properties(${APP_NAME} PROPERTIES
                        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/apple/macos/Info.plist"
                )
            elseif(IOS)
    #            cocos_pak_xcode(${APP_NAME} INFO_PLIST "iOSBundleInfo.plist.in")
    #            set_xcode_property(${APP_NAME} ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon")
    #            set_xcode_property(${APP_NAME} DEVELOPMENT_TEAM "")
    #            set_xcode_property(${APP_NAME} CODE_SIGN_IDENTITY "iPhone Developer")
            endif()
        elseif(WINDOWS)
            target_sources(${PROJECT_NAME} PRIVATE ${common_res_files})
            cocos_copy_target_dll(${APP_NAME})
        endif()

        if(LINUX OR WINDOWS)
            cocos_get_resource_path(APP_RES_DIR ${APP_NAME})
            add_custom_command(TARGET SYNC_RESOURCE-${APP_NAME} POST_BUILD
                COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/scripts/sync_runtime_assets.py"
                    --source "${GAME_RES_FOLDER}" --destination "${APP_RES_DIR}"
                    --all "${CMAKE_CURRENT_BINARY_DIR}/runtime-assets-all.txt"
                    --selected "${CMAKE_CURRENT_BINARY_DIR}/runtime-assets-selected.txt"
                VERBATIM)
        endif()
    endif()

endif() # KRKR2_BUILD_APP

if(KRKR2_BUILD_SDL3_PROTOTYPE)
    add_subdirectory(tools/sdl3-prototype)
endif()

if(ENABLE_TESTS AND NOT (IOS OR ANDROID))
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_TOOLS AND NOT (IOS OR ANDROID))
    add_subdirectory(tools)
endif()
