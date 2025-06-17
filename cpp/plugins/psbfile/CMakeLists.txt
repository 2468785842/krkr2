cmake_minimum_required(VERSION 3.19)
project(psbfile)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PUBLIC
    main.cpp
    PSBValue.cpp
    PSBFile.cpp
    types/PimgType.cpp
    resources/ImageMetadata.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE krkr2plugin)
