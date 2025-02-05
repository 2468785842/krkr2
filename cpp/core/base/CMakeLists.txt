set(BASE_PATH ${CMAKE_CURRENT_LIST_DIR})
set(BASE_SOURCE_FILES
    ${BASE_PATH}/7zArchive.cpp
    ${BASE_PATH}/BinaryStream.cpp
    ${BASE_PATH}/CharacterSet.cpp
    ${BASE_PATH}/EventIntf.cpp
    ${BASE_PATH}/PluginIntf.cpp
    ${BASE_PATH}/ScriptMgnIntf.cpp
    ${BASE_PATH}/StorageIntf.cpp
    ${BASE_PATH}/SysInitIntf.cpp
    ${BASE_PATH}/SystemIntf.cpp
    ${BASE_PATH}/TARArchive.cpp
    ${BASE_PATH}/TextStream.cpp
    ${BASE_PATH}/UtilStreams.cpp
    ${BASE_PATH}/XP3Archive.cpp
    ${BASE_PATH}/ZIPArchive.cpp

    ${BASE_PATH}/win32/EventImpl.cpp
    ${BASE_PATH}/win32/FileSelector.cpp
#    ${BASE_PATH}/win32/FuncStubs.cpp
    ${BASE_PATH}/win32/NativeEventQueue.cpp
    ${BASE_PATH}/win32/PluginImpl.cpp
    ${BASE_PATH}/win32/ScriptMgnImpl.cpp
    ${BASE_PATH}/win32/StorageImpl.cpp
#    ${BASE_PATH}/win32/SusieArchive.cpp
    ${BASE_PATH}/win32/SysInitImpl.cpp
    ${BASE_PATH}/win32/SystemImpl.cpp

    PARENT_SCOPE
)

set(BASE_HEADERS_DIR
    ${BASE_PATH}/
    ${BASE_PATH}/win32
    PARENT_SCOPE
)