cmake_minimum_required(VERSION 3.16)
project(TestTjs2 LANGUAGES CXX)

set(SOURCES
        tjs.cpp
        tjsString.cpp
)

if(KRKR2_TEST_SCOPE STREQUAL "strings")
    set(SOURCES tjsString.cpp)
    set(TJS_TEST_LIBRARY tjs2)
else()
    set(TJS_TEST_LIBRARY krkr2core)
endif()

string(REPLACE ".cpp" "" BASENAMES_SOURCES "${SOURCES}")
set(TARGETS ${BASENAMES_SOURCES})

foreach(name ${TARGETS})
    add_executable(${name} ${name}.cpp main.cpp)
endforeach()

set(ALL_TARGETS
        ${TARGETS}
)

foreach(name ${ALL_TARGETS})
    if(KRKR2_TEST_SCOPE STREQUAL "strings")
        target_compile_definitions(${name} PRIVATE KRKR2_TEST_STANDALONE_TJS)
    endif()
    target_link_libraries(${name}
        PRIVATE
            Catch2::Catch2
        PUBLIC
            ${TJS_TEST_LIBRARY}
    )
    target_include_directories(${name} PRIVATE "${TEST_CONFIG_DIR}" "${KRKR2CORE_PATH}")
    catch_discover_tests(${name})
endforeach()
