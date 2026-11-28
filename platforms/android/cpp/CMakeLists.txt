find_package(SDL3 3.4.8 EXACT CONFIG REQUIRED)
target_link_libraries(krkr2 PUBLIC SDL3::SDL3 core_config core_input)
target_include_directories(krkr2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_library(krkr2_settings SHARED Settings.cpp)
target_link_libraries(krkr2_settings PRIVATE core_launch core_config core_input)
