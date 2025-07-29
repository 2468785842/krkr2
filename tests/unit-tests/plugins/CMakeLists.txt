cmake_minimum_required(VERSION 3.16)
project(TestPlugins LANGUAGES CXX)

set(SOURCES
        psbfile-dll.cpp
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
            krkr2plugin krkr2core
    )
    target_include_directories(${name} PRIVATE "${TEST_CONFIG_DIR}")
    catch_discover_tests(${name})
endforeach()
