cmake_minimum_required(VERSION 3.19)
project(core_environ_module LANGUAGES CXX)

if(APPLE)
    enable_language(OBJC OBJCXX)
endif()

set(ENVIRON_PATH ${CMAKE_CURRENT_LIST_DIR})
set(ENVIRON_HEADERS_DIR
    ${ENVIRON_PATH}/
    ${ENVIRON_PATH}/ui
    ${ENVIRON_PATH}/ui/csd
    ${ENVIRON_PATH}/impl
    $<$<BOOL:${ANDROID}>:${ENVIRON_PATH}/android>
)

# cocos 无关的引擎环境层：平台胶水、全局配置、系统控制与桌面启动。
set(ENVIRON_SOURCE_FILES
    ${ENVIRON_PATH}/Application.cpp
    ${ENVIRON_PATH}/EnvironFileBackendInit.cpp
    ${ENVIRON_PATH}/DialogBackend.cpp
    ${ENVIRON_PATH}/DetectCPU.cpp
    ${ENVIRON_PATH}/XP3ArchiveRepack.cpp

    $<$<BOOL:${ANDROID}>:${ENVIRON_PATH}/linux/Platform.cpp>

    $<$<BOOL:${LINUX}>:${ENVIRON_PATH}/linux/Platform.cpp>

    ${ENVIRON_PATH}/ConfigManager/GlobalConfigManager.cpp

    ${ENVIRON_PATH}/impl/SystemControl.cpp
)

if(ANDROID OR IOS)
    list(APPEND ENVIRON_SOURCE_FILES
        ${ENVIRON_PATH}/MobileFileBackend.cpp
    )
endif()

if(NOT (ANDROID OR IOS))
    add_subdirectory(launch)
    list(APPEND ENVIRON_SOURCE_FILES ${ENVIRON_PATH}/launch/DesktopLaunch.cpp)
endif()

add_library(${PROJECT_NAME} STATIC ${ENVIRON_SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE core_input)
if(NOT (ANDROID OR IOS))
    target_link_libraries(${PROJECT_NAME} PRIVATE core_launch)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE
    KRKR2_USE_COCOS_UI=$<BOOL:${KRKR2_USE_COCOS_UI}>
    KRKR2_ENABLE_CRASH_UPLOAD=$<BOOL:${KRKR2_ENABLE_CRASH_UPLOAD}>)
target_include_directories(${PROJECT_NAME} PUBLIC ${ENVIRON_HEADERS_DIR})

# 组合根：对外 PUBLIC 链齐运行时栈（foundation + base + domain）
krkr2_target_link_foundation(${PROJECT_NAME} PUBLIC)
target_link_libraries(${PROJECT_NAME} PUBLIC
    core_base_module
    core_visual_module
    core_sound_module
    core_plugin_module
    core_movie_module
    core_utils_module
)

if(ANDROID OR IOS)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TVP_HAS_MOBILE_FILE_BACKEND)
endif()

find_package(7zip CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
        7zip::7zip
)

# ---------------------------------------------------------------------------
# cocos2d 后端源码，独立成库以便后续 SDL3 迁移门控。
# 所有直接或经头文件传递引用 cocos2dx 的环境层源码都在此目标编译。
# ---------------------------------------------------------------------------
set(ENVIRON_COCOS_SOURCE_FILES
    ${ENVIRON_PATH}/RenderUtils.cpp

    ${ENVIRON_PATH}/cocos2d/AppDelegate.cpp
    ${ENVIRON_PATH}/cocos2d/CCKeyCodeConv.cpp
    ${ENVIRON_PATH}/cocos2d/CustomFileUtils.cpp
    ${ENVIRON_PATH}/cocos2d/MainScene.cpp
    ${ENVIRON_PATH}/cocos2d/YUVSprite.cpp

    ${ENVIRON_PATH}/ConfigManager/PreferenceDefaults.cpp
    ${ENVIRON_PATH}/ConfigManager/IndividualConfigManager.cpp
    ${ENVIRON_PATH}/ConfigManager/LocaleConfigManager.cpp

    ${ENVIRON_PATH}/ui/csd/CsdUIFactory.cpp
    ${ENVIRON_PATH}/ui/csd/CsdUIWidgets.cpp
    ${ENVIRON_PATH}/ui/BaseForm.cpp
    ${ENVIRON_PATH}/ui/ConsoleWindow.cpp
    ${ENVIRON_PATH}/ui/DebugViewLayerForm.cpp
    ${ENVIRON_PATH}/ui/FileSelectorForm.cpp
    ${ENVIRON_PATH}/ui/GameMainMenu.cpp
    ${ENVIRON_PATH}/ui/InGameMenuForm.cpp
    ${ENVIRON_PATH}/ui/MessageBox.cpp
    ${ENVIRON_PATH}/ui/PreferenceForm.cpp
    ${ENVIRON_PATH}/ui/SelectListForm.cpp
    ${ENVIRON_PATH}/ui/SimpleMediaFilePlayer.cpp
    ${ENVIRON_PATH}/ui/TipsHelpForm.cpp
    ${ENVIRON_PATH}/ui/XP3RepackForm.cpp
    ${ENVIRON_PATH}/ui/extension/ActionExtension.cpp
    ${ENVIRON_PATH}/ui/extension/UIExtension.cpp

    ${ENVIRON_PATH}/impl/TVPWindow.cpp

    $<$<BOOL:${ANDROID}>:${ENVIRON_PATH}/android/AndroidUtils.cpp>

    $<$<BOOL:${WINDOWS}>:${ENVIRON_PATH}/win32/Platform.cpp>

    $<$<BOOL:${MACOS}>:${ENVIRON_PATH}/apple/macos/platform.mm>

    $<$<OR:$<BOOL:${ANDROID}>,$<BOOL:${IOS}>>:${ENVIRON_PATH}/CocosFileBackend.cpp>
)

if(KRKR2_USE_COCOS_UI)
    list(APPEND ENVIRON_COCOS_SOURCE_FILES
        ${ENVIRON_PATH}/ui/GlobalPreferenceForm.cpp
        ${ENVIRON_PATH}/ui/IndividualPreferenceForm.cpp
        ${ENVIRON_PATH}/ui/MainFileSelectorForm.cpp)
endif()

if(KRKR2_ENABLE_UI_TEST_DRIVER)
    list(APPEND ENVIRON_COCOS_SOURCE_FILES ${ENVIRON_PATH}/cocos2d/UIContractDriver.cpp)
    set_property(SOURCE ${ENVIRON_PATH}/cocos2d/MainScene.cpp APPEND PROPERTY
        COMPILE_DEFINITIONS KRKR2_ENABLE_UI_TEST_DRIVER=1)
endif()

if(KRKR2_ENABLE_CRASH_UPLOAD)
    list(APPEND ENVIRON_COCOS_SOURCE_FILES ${ENVIRON_PATH}/DumpSend.cpp)
endif()

add_library(core_environ_cocos STATIC ${ENVIRON_COCOS_SOURCE_FILES})
target_link_libraries(core_environ_cocos PRIVATE core_input)
if(NOT (ANDROID OR IOS))
    target_link_libraries(core_environ_cocos PRIVATE core_launch)
endif()
target_compile_definitions(core_environ_cocos PRIVATE
    KRKR2_USE_COCOS_UI=$<BOOL:${KRKR2_USE_COCOS_UI}>
    KRKR2_ENABLE_CRASH_UPLOAD=$<BOOL:${KRKR2_ENABLE_CRASH_UPLOAD}>)
target_include_directories(core_environ_cocos PUBLIC ${ENVIRON_HEADERS_DIR})

# 组合根：与 core_environ_module 链齐运行时栈
krkr2_target_link_foundation(core_environ_cocos PUBLIC)
target_link_libraries(core_environ_cocos PUBLIC
    core_base_module
    core_visual_module
    core_sound_module
    core_plugin_module
    core_movie_module
    core_utils_module
)

if(ANDROID OR IOS)
    target_compile_definitions(core_environ_cocos PRIVATE TVP_HAS_MOBILE_FILE_BACKEND)
endif()

find_package(cocos2dx CONFIG REQUIRED)

target_link_libraries(core_environ_cocos PRIVATE
        cocos2dx::cocos2d

        minizip
)

# ---------------------------------------------------------------------------
# SDL3 应用壳环境层（骨架，默认不构建；由 KRKR2_BUILD_SDL3_APP 门控）。
# 源文件由并行代理填充，落盘不完整时跳过本目标而非中断配置。
# ---------------------------------------------------------------------------
if(KRKR2_BUILD_SDL3_APP)
    if(EXISTS "${ENVIRON_PATH}/sdl3/SdlAppDelegate.cpp"
       AND EXISTS "${ENVIRON_PATH}/sdl3/SdlKeyCodeConv.cpp"
       AND EXISTS "${ENVIRON_PATH}/sdl3/SdlWindowLayer.cpp")
        add_subdirectory(sdl3)
    else()
        message(STATUS
            "core_environ_sdl3: skeleton sources incomplete under ${ENVIRON_PATH}/sdl3, "
            "skipping the SDL3 environ module")
    endif()
endif()
