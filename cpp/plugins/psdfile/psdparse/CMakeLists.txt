cmake_minimum_required(VERSION 3.19)
project(psdparse)

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

find_package(Boost COMPONENTS phoenix spirit iostreams REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
	krkr2plugin Boost::phoenix Boost::spirit Boost::iostreams
)