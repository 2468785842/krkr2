cmake_minimum_required(VERSION 3.19)
project(psdfile LANGUAGES CXX)

add_subdirectory(psdparse)

add_library(${PROJECT_NAME} STATIC
	main.cpp
	psdclass.cpp
	psdclass_loadmem.cpp
	psdclass_loadstream.cpp
	psdclass_loadstreambase.cpp
)

target_link_libraries(${PROJECT_NAME} PUBLIC psdparse)