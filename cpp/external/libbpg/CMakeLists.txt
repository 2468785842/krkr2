cmake_minimum_required(VERSION 3.15)
project(libbpg)

add_library(libbpg STATIC libbpg.c)
install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/"
        DESTINATION "include"
        FILES_MATCHING PATTERN "*.h"
)

target_include_directories(${PROJECT_NAME}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/>
        INTERFACE
            $<INSTALL_INTERFACE:include/>
)

find_package(FFMPEG COMPONENTS
        avutil
        avcodec REQUIRED
)
target_include_directories(${PROJECT_NAME} PRIVATE ${FFMPEG_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${FFMPEG_LIBRARIES})
