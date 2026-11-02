add_library(core_launch STATIC LaunchOptions.cpp)
target_compile_features(core_launch PUBLIC cxx_std_17)
target_include_directories(core_launch PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
