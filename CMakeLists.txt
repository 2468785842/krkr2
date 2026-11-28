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

# Detect the target before project(): vcpkg resolves the manifest in project().
get_filename_component(KRKR2_TOOLCHAIN_NAME "${CMAKE_TOOLCHAIN_FILE}" NAME)
if(IOS OR KRKR2_TOOLCHAIN_NAME MATCHES "ios"
   OR CMAKE_SYSTEM_NAME STREQUAL "iOS" OR VCPKG_TARGET_TRIPLET MATCHES "ios")
    message(FATAL_ERROR "iOS is temporarily disabled")
endif()
if(ANDROID OR CMAKE_SYSTEM_NAME STREQUAL "Android"
   OR KRKR2_TOOLCHAIN_NAME STREQUAL "android.toolchain.cmake"
   OR VCPKG_TARGET_TRIPLET MATCHES "-android$")
    set(ANDROID ON)
    include(cmake/AndroidToolchain.cmake)
endif()

option(KRKR2_ENABLE_UI_TEST_DRIVER "Build the opt-in desktop UI contract driver" OFF)
if(KRKR2_ENABLE_UI_TEST_DRIVER AND ANDROID)
    message(FATAL_ERROR "The UI contract driver is desktop-only")
endif()

option(KRKR2_BUILD_APP "Build the game runtime and bundled plugins" ON)
set(KRKR2_SCRIPT_BACKEND "tjs2" CACHE STRING "Script runtime: tjs2 or ciallang")
set_property(CACHE KRKR2_SCRIPT_BACKEND PROPERTY STRINGS tjs2 ciallang)
if(NOT KRKR2_SCRIPT_BACKEND MATCHES "^(tjs2|ciallang)$")
    message(FATAL_ERROR "KRKR2_SCRIPT_BACKEND must be tjs2 or ciallang")
endif()

if(CMAKE_SYSTEM_NAME)
    set(KRKR2_TARGET_SYSTEM "${CMAKE_SYSTEM_NAME}")
else()
    set(KRKR2_TARGET_SYSTEM "${CMAKE_HOST_SYSTEM_NAME}")
endif()
set(WINDOWS OFF)
set(LINUX OFF)
set(MACOS OFF)
if(ANDROID)
    add_compile_definitions(ANDROID)
    add_compile_options(-fsigned-char)
elseif(KRKR2_TARGET_SYSTEM STREQUAL "Windows")
    set(WINDOWS ON)
elseif(KRKR2_TARGET_SYSTEM STREQUAL "Linux")
    set(LINUX ON)
    add_compile_definitions(LINUX)
elseif(KRKR2_TARGET_SYSTEM STREQUAL "Darwin")
    set(MACOS ON)
else()
    message(FATAL_ERROR "Only Windows, Linux, macOS and Android are currently supported")
endif()

option(KRKR2_BUILD_SDL3_PROTOTYPE "Build the separate SDL3/TJS desktop shell experiment" OFF)
if(KRKR2_BUILD_SDL3_PROTOTYPE AND (ANDROID OR IOS))
    message(FATAL_ERROR "The SDL3 prototype is desktop-only")
endif()
option(ENABLE_TESTS "Build engine tests (desktop only)" ON)
option(BUILD_TOOLS "Build command-line tools (desktop only)" ON)
if(KRKR2_ENABLE_UI_TEST_DRIVER AND NOT KRKR2_BUILD_APP)
    message(FATAL_ERROR "The UI contract driver requires KRKR2_BUILD_APP=ON")
endif()
set(KRKR2_TEST_SCOPE "full" CACHE STRING "Test scope: full, strings, archives, imageops, launch, platform or config")
set_property(CACHE KRKR2_TEST_SCOPE PROPERTY STRINGS full strings archives imageops launch platform config)
if(NOT KRKR2_TEST_SCOPE MATCHES "^(full|strings|archives|imageops|launch|platform|config)$")
    message(FATAL_ERROR "KRKR2_TEST_SCOPE must be full, strings, archives, imageops, launch, platform or config")
endif()
option(KRKR2_ENABLE_PSD "Build the PSD plugin" ON)


# Full engine tests also compile the native host, even without an application.
set(KRKR2_ENABLE_SDL3_HOST OFF CACHE INTERNAL "Native host required by this configuration" FORCE)
if(KRKR2_BUILD_APP OR (ENABLE_TESTS AND KRKR2_TEST_SCOPE STREQUAL "full"))
    set(KRKR2_ENABLE_SDL3_HOST ON CACHE INTERNAL "Native host required by this configuration" FORCE)
endif()
# vcpkg installs dependencies during project(), before targets are configured.
set(VCPKG_MANIFEST_NO_DEFAULT_FEATURES ON)
if(KRKR2_BUILD_APP OR KRKR2_BUILD_SDL3_PROTOTYPE OR BUILD_TOOLS OR (ENABLE_TESTS AND NOT KRKR2_TEST_SCOPE MATCHES "^(imageops|launch|platform|config)$"))
    list(APPEND VCPKG_MANIFEST_FEATURES foundation)
endif()
if(KRKR2_BUILD_APP OR (ENABLE_TESTS AND KRKR2_TEST_SCOPE STREQUAL "full"))
    list(APPEND VCPKG_MANIFEST_FEATURES engine)
    if(KRKR2_ENABLE_PSD)
        list(APPEND VCPKG_MANIFEST_FEATURES psd)
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
    list(APPEND VCPKG_MANIFEST_FEATURES sdl3)
endif()

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
    set(CMAKE_TOOLCHAIN_FILE $ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake)
endif()

set(APP_NAME krkr2)

project(${APP_NAME})
if(NOT CMAKE_SYSTEM_NAME MATCHES "^(Windows|Linux|Darwin|Android)$")
    message(FATAL_ERROR "Only Windows, Linux, macOS and Android are currently supported")
endif()


set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)
if(APPLE)
    enable_language(OBJCXX)
    set(CMAKE_OBJCXX_STANDARD 17)
    set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
endif()
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
        add_library(${PROJECT_NAME} SHARED platforms/android/cpp/main.cpp)
    elseif(MACOS)
        add_executable(${PROJECT_NAME} MACOSX_BUNDLE platforms/macos/main.cpp)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE_GUI_IDENTIFIER "org.krkr2.runtime"
            MACOSX_BUNDLE_BUNDLE_NAME "KrKr2")
    elseif(LINUX)
        add_executable(${PROJECT_NAME} platforms/linux/sdl_main.cpp)
    else()
        add_executable(${PROJECT_NAME} platforms/windows/main.cpp platforms/windows/game.rc)
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
    set_target_properties(${PROJECT_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/krkr2")
    if(ANDROID)
        add_subdirectory(platforms/android/cpp)
    else()
        find_package(Python REQUIRED COMPONENTS Interpreter)
        set(GAME_RES_FOLDER "${CMAKE_CURRENT_SOURCE_DIR}/assets")
        include(cmake/RuntimeAssets.cmake)
        if(MACOS)
            set(KRKR2_RESOURCE_DESTINATION "$<TARGET_BUNDLE_CONTENT_DIR:krkr2>/Resources")
        else()
            set(KRKR2_RESOURCE_DESTINATION "$<TARGET_FILE_DIR:krkr2>/Resources")
        endif()
        add_custom_target(sync_sdl3_resources ALL
            COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/scripts/sync_runtime_assets.py"
                --source "${GAME_RES_FOLDER}" --destination "${KRKR2_RESOURCE_DESTINATION}"
                --all "${CMAKE_CURRENT_BINARY_DIR}/runtime-assets-all.txt"
                --selected "${CMAKE_CURRENT_BINARY_DIR}/runtime-assets-selected.txt"
                --retired "${CMAKE_CURRENT_SOURCE_DIR}/cmake/retired-runtime-assets.txt"
            VERBATIM)
        add_dependencies(${PROJECT_NAME} sync_sdl3_resources)
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
    if(LINUX)
        find_package(Threads REQUIRED)
        add_library(audio_worker_delay SHARED tools/game-smoke/delay_worker_threads.c)
        target_link_libraries(audio_worker_delay PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
        add_test(NAME audio-first-buffer-runtime
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/game-smoke/audio_startup_repro.py
                --engine $<TARGET_FILE:krkr2> --delay-library $<TARGET_FILE:audio_worker_delay>
                --output-dir ${CMAKE_BINARY_DIR}/script-smoke/audio-first-buffer)
        set_tests_properties(audio-first-buffer-runtime PROPERTIES TIMEOUT 35 LABELS "integration;audio")
        add_test(NAME image-audio-codec-runtime
            COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/game-smoke/audio_startup_repro.py
                --engine $<TARGET_FILE:krkr2> --delay-library $<TARGET_FILE:audio_worker_delay>
                --output-dir ${CMAKE_BINARY_DIR}/script-smoke/image-audio-codec
                --codec vorbis --webp)
        set_tests_properties(image-audio-codec-runtime PROPERTIES TIMEOUT 35 LABELS "integration;audio;image")
    endif()
endif()
