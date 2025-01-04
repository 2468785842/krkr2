cmake_minimum_required(VERSION 3.19)
project(psdparse)

project(${PROJECT_NAME})

find_package(Boost COMPONENTS phoenix spirit iostreams REQUIRED)

add_library(${PROJECT_NAME} STATIC
	bmp.cpp
	psddesc.cpp
	psdfile.cpp
	psdimage.cpp
	psdlayer.cpp
	psdparse.cpp
	psdresource.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
	${Boost_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PUBLIC krkr2plugin ${Boost_LIBRARIES})