cmake_minimum_required(VERSION 3.28)
project(krkr2core)

add_subdirectory(base)
add_subdirectory(environ)
add_subdirectory(movie)
add_subdirectory(msg)
add_subdirectory(sound)
add_subdirectory(visual)
add_subdirectory(tjs2)
add_subdirectory(utils)

# Create the static library
add_library(${PROJECT_NAME} STATIC
    ${BASE_SOURCE_FILES}
    ${UTILS_SOURCE_FILES}
    ${ENVIRON_SOURCE_FILES}
    ${KRKR2CORE_PATH}/extension/Extension.cpp
    ${MOVIE_SOURCE_FILES}
    ${MSG_SOURCE_FILES}
    ${SOUND_SOURCE_FILES}
    ${VISUAL_SOURCE_FILES}
)

# Common include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${BASE_HEADERS_DIR}
    ${UTILS_HEADERS_DIR}
    ${ENVIRON_HEADERS_DIR}
    ${KRKR2CORE_PATH}/extension
    ${MOVIE_HEADERS_DIR}
    ${MSG_HEADERS_DIR}
    ${SOUND_HEADERS_DIR}
    ${VISUAL_HEADERS_DIR}
    ${KRKR2CORE_PATH}/
    ${KRKR2CORE_PATH}/../plugins
)

target_compile_definitions(${PROJECT_NAME} PUBLIC
    -DTJS_TEXT_OUT_CRLF
    -D__STDC_CONSTANT_MACROS
    -DUSE_UNICODE_FSTRING
)

target_link_libraries(${PROJECT_NAME} PUBLIC tjs2 minizip)

# 启用 OpenMP 支持
find_package(OpenMP REQUIRED)

if(OPENMP_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
    target_compile_options(${PROJECT_NAME} PRIVATE ${OpenMP_CXX_FLAGS})
else()
    message(WARNING "OpenMP not found, fallback to single-threading.")
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
    log
    android
    EGL
    GLESv2
    GLESv1_CM
    OpenSLES
)

find_package(unofficial-breakpad CONFIG REQUIRED)

# oboe
if(DEFINED ANDROID)
    find_package (oboe CONFIG REQUIRED)
    target_link_libraries(${PROJECT_NAME} PUBLIC oboe::oboe)
endif()

# JXR library
find_package(JXR REQUIRED)
target_compile_definitions(${PROJECT_NAME} PUBLIC -D__ANSI__)
target_include_directories(${PROJECT_NAME} PUBLIC ${JXR_INCLUDE_DIRS})

find_package(JPEG REQUIRED)
find_package(libjpeg-turbo CONFIG REQUIRED)
find_package(oniguruma CONFIG REQUIRED)
find_package(libbpg CONFIG REQUIRED)
find_package(unrar CONFIG REQUIRED)
find_package(7zip CONFIG REQUIRED)
find_package(Opus CONFIG REQUIRED)
find_package(Ogg CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
find_package(OpusFile CONFIG REQUIRED)
find_package(lz4 CONFIG REQUIRED)

find_package(SDL2 CONFIG REQUIRED)

find_package(LibArchive REQUIRED)

find_package(OpenAL CONFIG REQUIRED)

set(OpenCV_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opencv4")
find_package(OpenCV REQUIRED)

find_package(FFMPEG COMPONENTS
    avutil
    avfilter
    avcodec
    swscale
    swresample REQUIRED
)
target_include_directories(${PROJECT_NAME} PUBLIC ${FFMPEG_INCLUDE_DIRS})

# cocos2dx
find_package(cocos2dx CONFIG REQUIRED)

# Link libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    ${FFMPEG_LIBRARIES}

    # audio
    Ogg::ogg
    Opus::opus
    Vorbis::vorbis
    Vorbis::vorbisfile
    Vorbis::vorbisenc
    OpusFile::opusfile
    OpenAL::OpenAL

    # image
    JPEG::JPEG
    $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>
    opencv_imgproc
    opencv_core

    # archive
    7zip::7zip
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

    # cocos2d
    cocos2dx::cocos2d
    $<$<BOOL:${ANDROID}>:$<LINK_LIBRARY:WHOLE_ARCHIVE,cocos2dx::cpp_android_spec>>
)