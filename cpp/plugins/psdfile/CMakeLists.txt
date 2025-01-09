cmake_minimum_required(VERSION 3.19)
project(psdfile)

add_subdirectory(psdparse)

add_library(${PROJECT_NAME} STATIC
    psdclass_loadmem.cpp
    psdclass_loadstream.cpp
    psdclass_loadstreambase.cpp
)

target_sources(${PROJECT_NAME} PUBLIC
    main.cpp
    psdclass.cpp
    psdclass.h
)

target_link_libraries(${PROJECT_NAME} PRIVATE krkr2plugin psdparse)