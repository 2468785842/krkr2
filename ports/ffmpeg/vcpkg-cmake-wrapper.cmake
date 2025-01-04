set(FFMPEG_PREV_MODULE_PATH ${CMAKE_MODULE_PATH})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

cmake_policy(SET CMP0012 NEW)

# Detect if we use "our" find module or a vendored one
set(z_vcpkg_using_vcpkg_find_ffmpeg OFF)

# Detect targets created e.g. by VTK/CMake/FindFFMPEG.cmake
set(vcpkg_no_avcodec_target ON)
set(vcpkg_no_avformat_target ON)
set(vcpkg_no_avutil_target ON)
set(vcpkg_no_swresample_target ON)
if(TARGET FFmpeg::avcodec)
  set(vcpkg_no_avcodec_target OFF)
endif()
if(TARGET FFmpeg::avformat)
  set(vcpkg_no_avformat_target OFF)
endif()
if(TARGET FFmpeg::avutil)
  set(vcpkg_no_avutil_target OFF)
endif()
if(TARGET FFmpeg::swresample)
  set(vcpkg_no_swresample_target OFF)
endif()

_find_package(${ARGS})

# Fixup of variables and targets for (some) vendored find modules
if(NOT z_vcpkg_using_vcpkg_find_ffmpeg)

  include(SelectLibraryConfigurations)

  if(CMAKE_HOST_WIN32)
    set(PKG_CONFIG_EXECUTABLE "${CMAKE_CURRENT_LIST_DIR}/../../../@_HOST_TRIPLET@/tools/pkgconf/pkgconf.exe" CACHE STRING "" FORCE)
  endif()

  set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON) # Required for CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 3.1 which otherwise ignores CMAKE_PREFIX_PATH

endif(NOT z_vcpkg_using_vcpkg_find_ffmpeg)
unset(z_vcpkg_using_vcpkg_find_ffmpeg)

set(FFMPEG_LIBRARY ${FFMPEG_LIBRARIES})
set(CMAKE_MODULE_PATH ${FFMPEG_PREV_MODULE_PATH})

unset(vcpkg_no_avcodec_target)
unset(vcpkg_no_avformat_target)
unset(vcpkg_no_avutil_target)
unset(vcpkg_no_swresample_target)