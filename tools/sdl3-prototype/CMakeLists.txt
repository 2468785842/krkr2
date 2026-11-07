cmake_minimum_required(VERSION 3.28)
project(krkr2_sdl3_prototype LANGUAGES CXX)

find_package(SDL3 3.4 CONFIG REQUIRED)
add_executable(krkr2_sdl3_prototype main.cpp SdlPointerInput.cpp PointerSelfTest.cpp)
target_compile_features(krkr2_sdl3_prototype PRIVATE cxx_std_17)
target_link_libraries(krkr2_sdl3_prototype PRIVATE SDL3::SDL3 tjs2 core_input)
