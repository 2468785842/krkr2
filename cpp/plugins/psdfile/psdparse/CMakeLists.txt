cmake_minimum_required(VERSION 3.19)

project(psdparse)

add_library(${PROJECT_NAME} STATIC
	bmp.cpp
	psddesc.cpp
	psdfile.cpp
	psdimage.cpp
	psdlayer.cpp
	psdparse.cpp
	psdresource.cpp
)

find_package(Boost COMPONENTS phoenix spirit iostreams REQUIRED NO_MODULE)

target_link_libraries(${PROJECT_NAME} PRIVATE
	krkr2plugin Boost::phoenix Boost::spirit Boost::iostreams
)