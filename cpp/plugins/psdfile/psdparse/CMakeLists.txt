cmake_minimum_required(VERSION 3.19)
project(psdparse LANGUAGES CXX)

project(${PROJECT_NAME})

add_library(${PROJECT_NAME} STATIC
	bmp.cpp
	psddesc.cpp
	psdfile.cpp
	psdimage.cpp
	psdlayer.cpp
	psdparse.cpp
	psdresource.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE krkr2plugin)