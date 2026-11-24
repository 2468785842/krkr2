cmake_minimum_required(VERSION 3.19)

if(NOT KRKR2_SDL3_ONLY)
    add_subdirectory(minizip)
endif()
add_subdirectory(libbpg)
