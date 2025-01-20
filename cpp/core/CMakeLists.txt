cmake_minimum_required(VERSION 3.19)
project(krkr2core)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB KRKR2CORE_SOURCES
    ${KRKR2CORE_PATH}/visual/*.cpp
    ${KRKR2CORE_PATH}/base/*.cpp
    ${KRKR2CORE_PATH}/base/win32/*.cpp
    ${KRKR2CORE_PATH}/environ/*.cpp
    ${KRKR2CORE_PATH}/environ/ConfigManager/*.cpp
    ${KRKR2CORE_PATH}/environ/android/*.cpp
    ${KRKR2CORE_PATH}/environ/cocos2d/*.cpp
    ${KRKR2CORE_PATH}/environ/linux/*.cpp
    ${KRKR2CORE_PATH}/environ/ui/*.cpp
    ${KRKR2CORE_PATH}/environ/ui/extension/*.cpp
    ${KRKR2CORE_PATH}/environ/win32/SystemControl.cpp
    ${KRKR2CORE_PATH}/extension/*.cpp
    ${KRKR2CORE_PATH}/movie/*.cpp
    ${KRKR2CORE_PATH}/movie/*/*.cpp
    ${KRKR2CORE_PATH}/msg/*.cpp
    ${KRKR2CORE_PATH}/msg/win32/MsgImpl.cpp
    ${KRKR2CORE_PATH}/msg/win32/OptionsDesc.cpp
    ${KRKR2CORE_PATH}/sound/*.cpp
    ${KRKR2CORE_PATH}/sound/win32/*.cpp
    ${KRKR2CORE_PATH}/tjs2/*.cpp
    ${KRKR2CORE_PATH}/utils/*.c
    ${KRKR2CORE_PATH}/utils/*.cpp
    ${KRKR2CORE_PATH}/utils/encoding/*.c
    ${KRKR2CORE_PATH}/utils/minizip/*.c
    ${KRKR2CORE_PATH}/utils/minizip/*.cpp
    ${KRKR2CORE_PATH}/utils/win32/*.cpp
    ${KRKR2CORE_PATH}/visual/gl/*.cpp
    ${KRKR2CORE_PATH}/visual/ogl/*.cpp
    ${KRKR2CORE_PATH}/visual/win32/*.cpp
)

file(GLOB KRKR2CORE_ANDROID_SOURCES
    ${KRKR2CORE_PATH}/environ/android/*.cpp
#    ${KRKR2CORE_PATH}/visual/ARM/*.cpp
)
list(REMOVE_ITEM KRKR2CORE_SOURCES
    ${KRKR2CORE_PATH}/visual/Resampler.cpp
    ${KRKR2CORE_PATH}/base/win32/FuncStubs.cpp
    ${KRKR2CORE_PATH}/base/win32/SusieArchive.cpp
    ${KRKR2CORE_PATH}/environ/MainFormUnit.cpp
    ${KRKR2CORE_PATH}/sound/xmmlib.cpp
    ${KRKR2CORE_PATH}/sound/WaveFormatConverter_SSE.cpp
    ${KRKR2CORE_PATH}/visual/win32/GDIFontRasterizer.cpp
    ${KRKR2CORE_PATH}/visual/win32/NativeFreeTypeFace.cpp
    ${KRKR2CORE_PATH}/visual/win32/TVPSysFont.cpp
    ${KRKR2CORE_PATH}/visual/win32/VSyncTimingThread.cpp
)

list(APPEND KRKR2CORE_SOURCES ${KRKR2CORE_ANDROID_SOURCES})
set(PLATFORM android)

# Create the static library
add_library(${PROJECT_NAME} STATIC ${KRKR2CORE_SOURCES})

# 启用 OpenMP 支持
find_package(OpenMP REQUIRED)

if(OPENMP_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
    target_compile_options(${PROJECT_NAME} PRIVATE ${OpenMP_CXX_FLAGS})
else()
    message(WARNING "OpenMP not found, fallback to single-threading.")
endif()

android_ndk_import_module_cpufeatures()

target_link_libraries(${PROJECT_NAME} PUBLIC
    -Wl,-Bdynamic
    log
    android
    EGL
    GLESv2
    GLESv1_CM
    OpenSLES
)

# Common include directories
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
    ${KRKR2CORE_PATH}/../plugins
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    -DTJS_TEXT_OUT_CRLF
    -D__STDC_CONSTANT_MACROS
    -DUSE_UNICODE_FSTRING
    -D_7ZIP_ST
    -DWINAPI=
)

find_package(unofficial-breakpad CONFIG REQUIRED)

# oboe
find_library(oboe_LIBPATH oboe REQUIRED)
add_library(oboe::oboe STATIC IMPORTED)
set_target_properties(oboe::oboe PROPERTIES IMPORTED_LOCATION ${oboe_LIBPATH})

# JXR library
find_package(JXR REQUIRED)
target_compile_definitions(${PROJECT_NAME} PUBLIC -D__ANSI__)
target_include_directories(${PROJECT_NAME} PUBLIC ${JXR_INCLUDE_DIRS})

find_package(JPEG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(oniguruma CONFIG REQUIRED)
find_package(libbpg CONFIG REQUIRED)
find_package(unrar CONFIG REQUIRED)
find_package(p7zip CONFIG REQUIRED)
find_package(Opus CONFIG REQUIRED)
find_package(Ogg CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
find_package(OpusFile CONFIG REQUIRED)
find_package(libbpg CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)

find_package(SDL2 CONFIG REQUIRED)

find_package(LibArchive REQUIRED)

find_package(OpenAL CONFIG REQUIRED)

set(OpenCV_ROOT "${VCPKG_INSTALLED_DIR}/arm64-android/share/opencv4")
find_package(OpenCV REQUIRED)
# target_include_directories(${PROJECT_NAME} PUBLIC ${OpenCV_INCLUDE_DIRS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    -Wl,-Bstatic

    # audio
    Ogg::ogg
    Opus::opus
    Vorbis::vorbis
    Vorbis::vorbisfile
    Vorbis::vorbisenc
    OpusFile::opusfile
    oboe::oboe
    OpenAL::OpenAL

    # image
    JPEG::JPEG
    $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>
    opencv_imgproc
    opencv_core
    # tegra_hal

    # archive
    p7zip::p7zip
    unrar::unrar
    lz4::lz4
    LibArchive::LibArchive
    libbpg::libbpg
    ${JXR_LIBRARIES}

    # others
    oniguruma::onig
    unofficial::breakpad::libbreakpad_client

    # framework
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)