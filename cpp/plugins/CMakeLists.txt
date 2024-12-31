cmake_minimum_required(VERSION 3.19)
project(krkr2plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_library(${PROJECT_NAME} STATIC
    scriptsEx.cpp
    InternalPlugins.cpp
    win32dialog.cpp
    dirlist.cpp
    csvParser.cpp
    layerExMovie.cpp
    varfile.cpp
    saveStruct.cpp
    getabout.cpp
    addFont.cpp
    wutcwf.cpp
    windowEx.cpp
    getSample.cpp
    layerExPerspective.cpp
    fftgraph.cpp
    LayerExBase.cpp
    xp3filter.cpp
    ncbind/ncbind.cpp
)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${KRKR2CORE_PATH}/
    ${KRKR2CORE_PATH}/base
    ${KRKR2CORE_PATH}/base/win32
    ${KRKR2CORE_PATH}/environ
    ${KRKR2CORE_PATH}/environ/win32
    ${KRKR2CORE_PATH}/environ/android
    ${KRKR2CORE_PATH}/environ/sdl
    ${KRKR2CORE_PATH}/msg
    ${KRKR2CORE_PATH}/msg/win32
    ${KRKR2CORE_PATH}/extension
    ${KRKR2CORE_PATH}/sound
    ${KRKR2CORE_PATH}/sound/win32
    ${KRKR2CORE_PATH}/tjs2
    ${KRKR2CORE_PATH}/utils
    ${KRKR2CORE_PATH}/utils/win32
    ${KRKR2CORE_PATH}/visual
    ${KRKR2CORE_PATH}/visual/ARM
    ${KRKR2CORE_PATH}/visual/win32
    ${KRKR2CORE_PATH}/visual/RenderScript/rs
)

add_definitions(-D__STDC_CONSTANT_MACROS -DTJS_TEXT_OUT_CRLF)

target_compile_options(${PROJECT_NAME} PRIVATE -fexceptions)

target_include_directories(${PROJECT_NAME} PUBLIC ${KRKR2PLUGIN_PATH} ${KRKR2PLUGIN_PATH}/ncbind)

target_include_directories(${PROJECT_NAME} PUBLIC ${FFMPEG_INCLUDE_DIRS})

target_include_directories(${PROJECT_NAME} PUBLIC ${COCOS2DX_EXTERNAL_HEADER_DIRS})

add_subdirectory(psdfile)
target_link_libraries(${PROJECT_NAME} PUBLIC psdfile)