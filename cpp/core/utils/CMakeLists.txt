add_subdirectory(minizip)

set(UTILS_PATH ${CMAKE_CURRENT_LIST_DIR})
set(UTILS_SOURCE_FILES
    ${UTILS_PATH}/ClipboardIntf.cpp
    ${UTILS_PATH}/DebugIntf.cpp
    ${UTILS_PATH}/MathAlgorithms_Default.cpp
    ${UTILS_PATH}/md5.c
    ${UTILS_PATH}/MiscUtility.cpp
    ${UTILS_PATH}/PadIntf.cpp
    ${UTILS_PATH}/Random.cpp
    ${UTILS_PATH}/RealFFT_Default.cpp
    ${UTILS_PATH}/ThreadIntf.cpp
    ${UTILS_PATH}/TickCount.cpp
    ${UTILS_PATH}/TimerIntf.cpp
    ${UTILS_PATH}/VelocityTracker.cpp

    ${UTILS_PATH}/encoding/gbk2unicode.c
    ${UTILS_PATH}/encoding/jis2unicode.c

    ${UTILS_PATH}/win32/ClipboardImpl.cpp
    ${UTILS_PATH}/win32/DebugImpl.cpp
    ${UTILS_PATH}/win32/PadImpl.cpp
    ${UTILS_PATH}/win32/ThreadImpl.cpp
    ${UTILS_PATH}/win32/TimerImpl.cpp
    ${UTILS_PATH}/win32/TVPTimer.cpp
    PARENT_SCOPE
)

set(UTILS_HEADERS_DIR
    ${UTILS_PATH}/
    ${UTILS_PATH}/iconv/
    ${UTILS_PATH}/win32/
    PARENT_SCOPE
)