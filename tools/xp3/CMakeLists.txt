cmake_minimum_required(VERSION 3.28)
project(xp3 LANGUAGES CXX VERSION 1.0.0)

add_executable(${PROJECT_NAME}
    main.cpp
)

find_package(argparse CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE argparse::argparse)

target_link_libraries(${PROJECT_NAME} PRIVATE krkr2core)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PROGRAM_NAME="${PROJECT_NAME}"
    VERSION="${PROJECT_VERSION}"
)

