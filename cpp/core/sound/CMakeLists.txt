cmake_minimum_required(VERSION 3.19)
project(core_sound_module LANGUAGES CXX)

set(SOUND_PATH ${CMAKE_CURRENT_LIST_DIR})
set(SOUND_SOURCE_FILES
    ${SOUND_PATH}/CDDAIntf.cpp
    ${SOUND_PATH}/FFWaveDecoder.cpp
    ${SOUND_PATH}/MathAlgorithms.cpp
    ${SOUND_PATH}/MIDIIntf.cpp
    ${SOUND_PATH}/PhaseVocoderDSP.cpp
    ${SOUND_PATH}/PhaseVocoderFilter.cpp
    ${SOUND_PATH}/SoundBufferBaseIntf.cpp
    ${SOUND_PATH}/VorbisWaveDecoder.cpp
    ${SOUND_PATH}/WaveFormatConverter.cpp
#    ${SOUND_PATH}/WaveFormatConverter_SSE.cpp
    ${SOUND_PATH}/WaveIntf.cpp
    ${SOUND_PATH}/WaveLoopManager.cpp
    ${SOUND_PATH}/WaveSegmentQueue.cpp
#    ${SOUND_PATH}/xmmlib.cpp

    ${SOUND_PATH}/win32/CDDAImpl.cpp
    ${SOUND_PATH}/win32/MIDIImpl.cpp
    ${SOUND_PATH}/win32/SoundBufferBaseImpl.cpp
#    ${SOUND_PATH}/win32/tvpsnd.c
    ${SOUND_PATH}/win32/tvpsnd.cpp
#    ${SOUND_PATH}/win32/tvpsnd.idl
    ${SOUND_PATH}/win32/WaveImpl.cpp
    ${SOUND_PATH}/win32/WaveMixer.cpp
    ${SOUND_PATH}/win32/WaveMixerConverter.cpp
)

set(SOUND_HEADERS_DIR
    ${SOUND_PATH}/
    ${SOUND_PATH}/win32
)

if(ANDROID)
    find_package(oboe CONFIG REQUIRED)
    list(APPEND SOUND_SOURCE_FILES ${SOUND_PATH}/win32/WaveMixerMiniaudio.cpp)
endif()

add_library(${PROJECT_NAME} STATIC ${SOUND_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${SOUND_HEADERS_DIR})
krkr2_target_link_foundation(${PROJECT_NAME} PUBLIC)
target_link_libraries(${PROJECT_NAME} PRIVATE core_utils_module)
krkr2_target_include_base_headers(${PROJECT_NAME})
krkr2_target_include_environ_headers(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${KRKR2_CORE_DIR}/movie
    ${KRKR2_CORE_DIR}/movie/ffmpeg
)

find_package(Vorbis CONFIG REQUIRED)
find_package(Opus CONFIG REQUIRED)
find_package(OpusFile CONFIG REQUIRED)

if(ANDROID)
    find_path(MINIAUDIO_INCLUDE_DIR NAMES miniaudio.h REQUIRED)
    target_include_directories(${PROJECT_NAME} PRIVATE ${MINIAUDIO_INCLUDE_DIR})

    set_source_files_properties(
        ${SOUND_PATH}/win32/WaveMixerMiniaudio.cpp
        PROPERTIES
        COMPILE_DEFINITIONS
        "MINIAUDIO_IMPLEMENTATION;MA_NO_DECODING;MA_NO_ENCODING;MA_NO_RESOURCE_MANAGER;MA_NO_NODE_GRAPH"
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        oboe::oboe
    )
endif()

find_package(OpenAL CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Vorbis::vorbis
    Vorbis::vorbisfile
    Vorbis::vorbisenc

    Opus::opus
    OpusFile::opusfile
    OpenAL::OpenAL
)
