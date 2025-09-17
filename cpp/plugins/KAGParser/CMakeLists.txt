cmake_minimum_required(VERSION 3.19)

project(KAGParser)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PUBLIC
        KAGParser.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC
        krkr2plugin
)
