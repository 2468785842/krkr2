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

option(KRKR2_ENABLE_UI_TEST_DRIVER "Build the opt-in desktop UI contract driver" OFF)
if(KRKR2_ENABLE_UI_TEST_DRIVER AND (ANDROID OR IOS))
    message(FATAL_ERROR "The UI contract driver is desktop-only")
endif()

option(KRKR2_BUILD_APP "Build the game runtime and bundled plugins" ON)
option(KRKR2_ENABLE_SDL3_HOST "Enable the SDL3 desktop game host" OFF)
# Compatibility for existing Linux build directories; new presets use SDL3_ONLY.
option(KRKR2_COCOS_FREE "Compatibility alias for KRKR2_SDL3_ONLY" OFF)
if(KRKR2_COCOS_FREE)
    set(KRKR2_SDL3_ONLY ON CACHE BOOL "Build SDL3 without Cocos" FORCE)
endif()
option(KRKR2_SDL3_ONLY "Build the Windows/Linux SDL3 runtime without Cocos" OFF)
if(KRKR2_SDL3_ONLY)
    if((NOT WINDOWS AND NOT LINUX) OR NOT KRKR2_BUILD_APP OR ANDROID OR IOS OR KRKR2_ENABLE_CRASH_UPLOAD)
        message(FATAL_ERROR "SDL3-only requires the Windows or Linux application without crash upload")
    endif()
    set(KRKR2_ENABLE_SDL3_HOST ON CACHE BOOL "Enable the SDL3 desktop game host" FORCE)
    add_compile_definitions(KRKR2_SDL3_ONLY=1)
endif()
if(WINDOWS AND KRKR2_ENABLE_SDL3_HOST AND NOT KRKR2_SDL3_ONLY)
    message(FATAL_ERROR "Windows SDL3 requires KRKR2_SDL3_ONLY=ON")
endif()
if(KRKR2_ENABLE_SDL3_HOST AND (NOT KRKR2_BUILD_APP OR (NOT LINUX AND NOT WINDOWS) OR ANDROID OR IOS))
    message(FATAL_ERROR "The SDL3 host requires the Windows or Linux game runtime")
endif()
option(KRKR2_BUILD_SDL3_PROTOTYPE "Build the separate SDL3/TJS desktop shell experiment" OFF)
if(KRKR2_BUILD_SDL3_PROTOTYPE AND (ANDROID OR IOS))
    message(FATAL_ERROR "The SDL3 prototype is desktop-only")
endif()
option(ENABLE_TESTS "Build engine tests (desktop only)" ON)
option(BUILD_TOOLS "Build command-line tools (desktop only)" ON)
if(KRKR2_ENABLE_UI_TEST_DRIVER AND (NOT KRKR2_BUILD_APP OR (NOT ENABLE_TESTS AND NOT KRKR2_SDL3_ONLY)))
    message(FATAL_ERROR "The UI contract driver requires KRKR2_BUILD_APP=ON and ENABLE_TESTS=ON")
endif()
set(KRKR2_TEST_SCOPE "full" CACHE STRING "Test scope: full, strings, archives, imageops, launch, platform or config")
set_property(CACHE KRKR2_TEST_SCOPE PROPERTY STRINGS full strings archives imageops launch platform config)
if(NOT KRKR2_TEST_SCOPE MATCHES "^(full|strings|archives|imageops|launch|platform|config)$")
    message(FATAL_ERROR "KRKR2_TEST_SCOPE must be full, strings, archives, imageops, launch, platform or config")
endif()
option(KRKR2_ENABLE_PSD "Build the PSD plugin" ON)
option(KRKR2_ENABLE_CRASH_UPLOAD "Build optional crash report upload" OFF)

# vcpkg installs dependencies during project(), before targets are configured.
set(VCPKG_MANIFEST_NO_DEFAULT_FEATURES ON)
if(KRKR2_BUILD_APP OR KRKR2_BUILD_SDL3_PROTOTYPE OR BUILD_TOOLS OR (ENABLE_TESTS AND NOT KRKR2_TEST_SCOPE MATCHES "^(imageops|launch|platform|config)$"))
    list(APPEND VCPKG_MANIFEST_FEATURES foundation)
endif()
if(KRKR2_BUILD_APP OR (ENABLE_TESTS AND KRKR2_TEST_SCOPE STREQUAL "full"))
    list(APPEND VCPKG_MANIFEST_FEATURES engine)
    if(NOT KRKR2_SDL3_ONLY)
        list(APPEND VCPKG_MANIFEST_FEATURES cocos)
    endif()
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
    elseif(KRKR2_TEST_SCOPE STREQUAL "config")
        list(APPEND VCPKG_MANIFEST_FEATURES config)
    elseif(KRKR2_TEST_SCOPE STREQUAL "imageops")
        list(APPEND VCPKG_MANIFEST_FEATURES imageops)
    endif()
endif()
if(BUILD_TOOLS AND NOT (ANDROID OR IOS))
    list(APPEND VCPKG_MANIFEST_FEATURES tools)
endif()
list(REMOVE_DUPLICATES VCPKG_MANIFEST_FEATURES)
if(KRKR2_ENABLE_SDL3_HOST AND LINUX)
    list(APPEND VCPKG_MANIFEST_FEATURES native-dialogs)
endif()
if(KRKR2_BUILD_SDL3_PROTOTYPE OR KRKR2_ENABLE_SDL3_HOST)
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

if(ANDROID OR IOS)
    set(KRKR2_USE_COCOS_UI ON)
else()
    set(KRKR2_USE_COCOS_UI OFF)
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(KRKR2_SDL3_ONLY)
    if(LINUX)
        add_compile_definitions(LINUX)
    endif()
endif()

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
    if(KRKR2_BUILD_SDL3_PROTOTYPE OR BUILD_TOOLS OR NOT KRKR2_TEST_SCOPE MATCHES "^(imageops|launch|platform|config)$")
        add_subdirectory(cpp/core/log)
        add_subdirectory(cpp/core/tjs2)
    endif()
    if(KRKR2_BUILD_SDL3_PROTOTYPE OR (ENABLE_TESTS AND KRKR2_TEST_SCOPE STREQUAL "platform"))
        add_subdirectory(cpp/core/input)
    endif()
    if(KRKR2_BUILD_SDL3_PROTOTYPE)
        add_subdirectory(tools/sdl3-prototype)
    endif()
    if(ENABLE_TESTS AND KRKR2_TEST_SCOPE STREQUAL "config")
        add_subdirectory(cpp/core/environ/resources)
        add_subdirectory(cpp/core/environ/ConfigManager)
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
        elseif(KRKR2_TEST_SCOPE STREQUAL "config")
            add_subdirectory(tests/unit-tests/core/config)
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
        if(KRKR2_SDL3_ONLY)
            add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux/sdl_main.cpp)
        else()
            add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/platforms/linux/main.cpp)
        endif()
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

    if(NOT KRKR2_SDL3_ONLY)
        target_include_directories(${PROJECT_NAME} PUBLIC ${KRKR2CORE_PATH}/environ/cocos2d)
    endif()

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
    if(KRKR2_ENABLE_SDL3_HOST)
        target_compile_definitions(${PROJECT_NAME} PRIVATE KRKR2_ENABLE_SDL3_HOST=1)
    endif()
    if(ANDROID)
        # The JNI bridge consumes the text codec directly; environ's PRIVATE
        # dependency does not export its headers to this application target.
        target_link_libraries(${PROJECT_NAME} PRIVATE core_input)
    endif()

    ## resources
    if(KRKR2_SDL3_ONLY)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/krkr2")
        find_package(Python REQUIRED COMPONENTS Interpreter)
        set(GAME_RES_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/assets")
        include(cmake/RuntimeAssets.cmake)
        add_custom_target(sync_sdl3_resources ALL
            COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/scripts/sync_runtime_assets.py"
                --source "${GAME_RES_FOLDER}" --destination "$<TARGET_FILE_DIR:krkr2>/Resources"
                --all "${CMAKE_CURRENT_BINARY_DIR}/runtime-assets-all.txt"
                --selected "${CMAKE_CURRENT_BINARY_DIR}/runtime-assets-selected.txt"
            VERBATIM)
        add_dependencies(${PROJECT_NAME} sync_sdl3_resources)
    elseif(NOT ANDROID)
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

# Opt-in application test; requires a desktop display (xvfb-run on Linux).
option(KRKR2_ENABLE_PSBFILE_SMOKE "Register synthetic PSBFile TJS integration test" OFF)
if(KRKR2_ENABLE_PSBFILE_SMOKE)
    if(NOT TARGET krkr2 OR ANDROID OR IOS)
        message(FATAL_ERROR "PSBFile smoke requires the desktop krkr2 application")
    endif()
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME psbfile-runtime
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/psbfile/smoke.py"
                --engine "$<TARGET_FILE:krkr2>")
    set_tests_properties(psbfile-runtime PROPERTIES TIMEOUT 75 LABELS "integration;psbfile")
    add_test(NAME layer-alpha-runtime
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/game-smoke/layer_alpha.py"
                --engine "$<TARGET_FILE:krkr2>")
    set_tests_properties(layer-alpha-runtime PROPERTIES TIMEOUT 75 LABELS "integration;graphics;plugin")
endif()

option(KRKR2_ENABLE_X11_INPUT_SMOKE "Register X11 keyboard integration test" OFF)
if(KRKR2_ENABLE_X11_INPUT_SMOKE)
    if(NOT TARGET krkr2 OR NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR ANDROID)
        message(FATAL_ERROR "X11 input smoke requires the Linux desktop krkr2 application")
    endif()
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    find_program(KRKR2_XDOTOOL xdotool REQUIRED)
    add_test(NAME x11-keyboard-runtime
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/game-smoke/keyboard_x11.py"
                --engine "$<TARGET_FILE:krkr2>" --xdotool "${KRKR2_XDOTOOL}")
    set_tests_properties(x11-keyboard-runtime PROPERTIES TIMEOUT 75 LABELS "integration;input;x11")
    add_test(NAME x11-controller-focus-runtime
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/game-smoke/controller_focus_x11.py"
                --engine "$<TARGET_FILE:krkr2>" --xdotool "${KRKR2_XDOTOOL}")
    set_tests_properties(x11-controller-focus-runtime PROPERTIES TIMEOUT 75 LABELS "integration;input;x11")
endif()

# Opt-in application tests run the real engine with synthetic scripts only.
option(KRKR2_ENABLE_SCRIPT_SMOKE "Register desktop script integration tests" OFF)
if(KRKR2_ENABLE_SCRIPT_SMOKE)
    if(NOT TARGET krkr2 OR ANDROID OR IOS)
        message(FATAL_ERROR "Script smoke requires the desktop krkr2 application")
    endif()
    enable_testing()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME textstream-runtime
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/game-smoke/textstream_repro.py"
                --engine "$<TARGET_FILE:krkr2>"
                --output-dir "${CMAKE_CURRENT_BINARY_DIR}/script-smoke/textstream")
    add_test(NAME parser-error-runtime
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/game-smoke/parser_error_repro.py"
                --engine "$<TARGET_FILE:krkr2>"
                --output-dir "${CMAKE_CURRENT_BINARY_DIR}/script-smoke/parser-error")
    add_test(NAME renderer-error-runtime
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/game-smoke/renderer_error_repro.py"
                --engine "$<TARGET_FILE:krkr2>"
                --output-dir "${CMAKE_CURRENT_BINARY_DIR}/script-smoke/renderer-error")
    set_tests_properties(textstream-runtime parser-error-runtime renderer-error-runtime
        PROPERTIES TIMEOUT 120 LABELS "integration;script")
endif()
