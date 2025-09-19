cmake_minimum_required(VERSION 3.16)
project(TestMovieModule LANGUAGES CXX)

set(SOURCES
        ffmpeg.cpp
)

string(REPLACE ".cpp" "" BASENAMES_SOURCES "${SOURCES}")
set(TARGETS ${BASENAMES_SOURCES})

foreach(name ${TARGETS})
    add_executable(${name} ${name}.cpp main.cpp)
endforeach()

set(ALL_TARGETS
        ${TARGETS}
)

foreach(name ${ALL_TARGETS})
    target_link_libraries(${name}
        PRIVATE
            Catch2::Catch2
        PUBLIC
            core_movie_module
    )
    target_include_directories(${name} PRIVATE "${TEST_CONFIG_DIR}")
    catch_discover_tests(${name})
endforeach()
