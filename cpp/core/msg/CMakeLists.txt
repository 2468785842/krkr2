set(MSG_PATH ${CMAKE_CURRENT_LIST_DIR})
set(MSG_SOURCE_FILES
    ${MSG_PATH}/MsgIntf.cpp

    ${MSG_PATH}/win32/MsgImpl.cpp
    ${MSG_PATH}/win32/OptionsDesc.cpp
#    ${MSG_PATH}/win32/MsgLoad.cpp
#    ${MSG_PATH}/win32/ReadOptionDesc.cpp

    PARENT_SCOPE
)

set(MSG_HEADERS_DIR
    ${MSG_PATH}/
    ${MSG_PATH}/win32
    PARENT_SCOPE
)