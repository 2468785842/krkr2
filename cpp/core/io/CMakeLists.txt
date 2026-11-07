cmake_minimum_required(VERSION 3.19)
project(core_io LANGUAGES CXX)

set(IO_PATH ${CMAKE_CURRENT_LIST_DIR})
set(IO_SOURCE_FILES
    ${IO_PATH}/IFileBackend.cpp
    ${IO_PATH}/FileBackendRegistry.cpp
    ${IO_PATH}/LocalFileStream.cpp
)

add_library(${PROJECT_NAME} STATIC ${IO_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${IO_PATH})
target_link_libraries(${PROJECT_NAME} PUBLIC tjs2 core_msg)
