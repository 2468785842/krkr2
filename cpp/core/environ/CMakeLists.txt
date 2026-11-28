cmake_minimum_required(VERSION 3.19)
project(core_environ_module LANGUAGES CXX)

set(ENVIRON_PATH ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(resources)
add_subdirectory(ConfigManager)
add_subdirectory(launch)

add_library(${PROJECT_NAME} STATIC
    RenderHost.cpp
    HostFrame.cpp
    Application.cpp
    EnvironFileBackendInit.cpp
    DialogBackend.cpp
    DetectCPU.cpp
    XP3ArchiveRepack.cpp
    HostPaths.cpp
    impl/SystemControl.cpp
    launch/DesktopLaunch.cpp
    sdl3/NativeServices.cpp
    sdl3/SdlHost.cpp
    sdl3/SdlGamepads.cpp
)

if(ANDROID)
    target_sources(${PROJECT_NAME} PRIVATE
        platform/android/Platform.cpp
        sdl3/android/NativeHost.cpp
    )
    target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/platforms/android/cpp)
elseif(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE
        platform/windows/Platform.cpp
        sdl3/windows/HostServices.cpp
        sdl3/windows/NativeHost.cpp
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(${PROJECT_NAME} PRIVATE
        platform/linux/Platform.cpp
        sdl3/linux/HostServices.cpp
        sdl3/linux/NativeDialogs.cpp
        sdl3/linux/NativeHost.cpp
    )
elseif(APPLE)
    target_sources(${PROJECT_NAME} PRIVATE
        platform/macos/Platform.mm
        sdl3/macos/HostServices.mm
        sdl3/macos/NativeHost.mm
    )
    target_compile_options(${PROJECT_NAME} PRIVATE "$<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>")
    find_library(KRKR2_COCOA_FRAMEWORK Cocoa REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${KRKR2_COCOA_FRAMEWORK})
else()
    message(FATAL_ERROR "Unsupported environ platform: ${CMAKE_SYSTEM_NAME}")
endif()

if(KRKR2_ENABLE_UI_TEST_DRIVER)
    target_sources(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/tools/game-smoke/SdlGamepadContractDriver.cpp
        ${CMAKE_SOURCE_DIR}/tools/game-smoke/SdlTouchContractDriver.cpp
    )
    set_property(SOURCE sdl3/SdlHost.cpp APPEND PROPERTY
        COMPILE_DEFINITIONS KRKR2_ENABLE_UI_TEST_DRIVER=1
    )
endif()

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${ENVIRON_PATH}
        ${ENVIRON_PATH}/impl
    PRIVATE
        ${ENVIRON_PATH}/sdl3
)
target_link_libraries(${PROJECT_NAME} PRIVATE
    core_input
    core_host_assets
    core_config
    core_launch
)
krkr2_target_link_foundation(${PROJECT_NAME} PUBLIC)
target_link_libraries(${PROJECT_NAME} PUBLIC
    core_base_module
    core_visual_module
    core_sound_module
    core_plugin_module
    core_movie_module
    core_utils_module
)

find_package(SDL3 3.4 CONFIG REQUIRED)
target_compile_definitions(${PROJECT_NAME} PRIVATE KRKR2_ENABLE_SDL3_HOST=1)
target_link_libraries(${PROJECT_NAME} PRIVATE SDL3::SDL3)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(NATIVE_GTK REQUIRED IMPORTED_TARGET gtk+-3.0)
    target_link_libraries(${PROJECT_NAME} PRIVATE PkgConfig::NATIVE_GTK)
endif()

find_package(7zip CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE 7zip::7zip)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        winmm
        comdlg32
        shell32
        ole32
    )
endif()
