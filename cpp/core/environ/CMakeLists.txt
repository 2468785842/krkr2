set(ENVIRON_PATH ${CMAKE_CURRENT_LIST_DIR})
set(ENVIRON_SOURCE_FILES
    ${ENVIRON_PATH}/Application.cpp
    ${ENVIRON_PATH}/DetectCPU.cpp
    ${ENVIRON_PATH}/DumpSend.cpp
    ${ENVIRON_PATH}/XP3ArchiveRepack.cpp

    ${ENVIRON_PATH}/android/AndroidUtils.cpp

    ${ENVIRON_PATH}/cocos2d/AppDelegate.cpp
    ${ENVIRON_PATH}/cocos2d/CCKeyCodeConv.cpp
    ${ENVIRON_PATH}/cocos2d/CustomFileUtils.cpp
    ${ENVIRON_PATH}/cocos2d/MainScene.cpp
    ${ENVIRON_PATH}/cocos2d/YUVSprite.cpp

    ${ENVIRON_PATH}/ConfigManager/GlobalConfigManager.cpp
    ${ENVIRON_PATH}/ConfigManager/IndividualConfigManager.cpp
    ${ENVIRON_PATH}/ConfigManager/LocaleConfigManager.cpp
    ${ENVIRON_PATH}/linux/Platform.cpp
    ${ENVIRON_PATH}/sdl/tvpsdl.cpp

    ${ENVIRON_PATH}/ui/BaseForm.cpp
    ${ENVIRON_PATH}/ui/ConsoleWindow.cpp
    ${ENVIRON_PATH}/ui/DebugViewLayerForm.cpp
    ${ENVIRON_PATH}/ui/FileSelectorForm.cpp
    ${ENVIRON_PATH}/ui/GameMainMenu.cpp
    ${ENVIRON_PATH}/ui/GlobalPreferenceForm.cpp
    ${ENVIRON_PATH}/ui/IndividualPreferenceForm.cpp
    ${ENVIRON_PATH}/ui/MainFileSelectorForm.cpp
    ${ENVIRON_PATH}/ui/InGameMenuForm.cpp
    ${ENVIRON_PATH}/ui/MessageBox.cpp
    ${ENVIRON_PATH}/ui/PreferenceForm.cpp
    ${ENVIRON_PATH}/ui/SelectListForm.cpp
    ${ENVIRON_PATH}/ui/SimpleMediaFilePlayer.cpp
    ${ENVIRON_PATH}/ui/TipsHelpForm.cpp
    ${ENVIRON_PATH}/ui/XP3RepackForm.cpp
    ${ENVIRON_PATH}/ui/extension/ActionExtension.cpp
    ${ENVIRON_PATH}/ui/extension/UIExtension.cpp

    ${ENVIRON_PATH}/win32/SystemControl.cpp
#    ${ENVIRON_PATH}/win32/CompatibleNativeFuncs.cpp
#    ${ENVIRON_PATH}/win32/ConfigFormUnit.cpp
#    ${ENVIRON_PATH}/win32/DetectCPU.cpp
#    ${ENVIRON_PATH}/win32/EmergencyExit.cpp
#    ${ENVIRON_PATH}/win32/HitWindow.cpp
#    ${ENVIRON_PATH}/win32/MouseCursor.cpp
#    ${ENVIRON_PATH}/win32/my_HintWindow.cpp
#    ${ENVIRON_PATH}/win32/Platform.cpp
#    ${ENVIRON_PATH}/win32/TouchPoint.cpp
#    ${ENVIRON_PATH}/win32/TVPWindow.cpp
#    ${ENVIRON_PATH}/win32/VersionFormUnit.cpp
#    ${ENVIRON_PATH}/win32/WindowFormUnit.cpp
#    ${ENVIRON_PATH}/win32/WindowsUtil.cpp

    PARENT_SCOPE
)

set(ENVIRON_HEADERS_DIR
    ${ENVIRON_PATH}/
    ${ENVIRON_PATH}/win32
    ${ENVIRON_PATH}/android
    ${ENVIRON_PATH}/sdl
    PARENT_SCOPE
)