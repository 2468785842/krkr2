cmake_minimum_required(VERSION 3.19)
project(core_input LANGUAGES CXX)

add_library(core_input STATIC InputState.cpp InputModifiers.cpp PointerCoordinates.cpp)
target_compile_features(core_input PUBLIC cxx_std_17)
target_include_directories(core_input PUBLIC ${CMAKE_CURRENT_LIST_DIR})
# Standard C++ leaf: no engine or backend link dependencies.
