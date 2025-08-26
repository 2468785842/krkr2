cmake_minimum_required(VERSION 3.19)

## UTF-8 source
#add_compile_options("$<$<AND:$<C_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:C>>:/utf-8>")
#add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/utf-8>")
#add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/Zc:__cplusplus>")
#
## MSVC static runtime
#set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
project(KAGParserEx)

#if(NOT TARGET tp_stub)
#add_subdirectory(../tp_stub ${CMAKE_CURRENT_BINARY_DIR}/tp_stub)
#endif()


add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PUBLIC
        KAGParser.cpp
        KAGParser.h
        KAGParserEx.cpp
        KAGParserEx.hpp
        Main.cpp
        tjsHashSearch.h
)

target_link_libraries(${PROJECT_NAME} PUBLIC
        krkr2plugin
)
