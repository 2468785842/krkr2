find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(ZLIB REQUIRED)

set(ARCHIVE_FIXTURES ${CMAKE_CURRENT_BINARY_DIR}/fixtures)
add_custom_command(
    OUTPUT ${ARCHIVE_FIXTURES}/ready.stamp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_LIST_DIR}/make_fixtures.py ${ARCHIVE_FIXTURES}
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/make_fixtures.py
    VERBATIM
)
add_custom_target(archive_fixtures DEPENDS ${ARCHIVE_FIXTURES}/ready.stamp)

add_executable(archive_tests ArchiveTest.cpp HeadlessRuntime.cpp)
add_dependencies(archive_tests archive_fixtures)
target_link_libraries(archive_tests PRIVATE Catch2::Catch2 archive_zip archive_tar)
if(TARGET archive_xp3)
    target_link_libraries(archive_tests PRIVATE archive_xp3)
    target_compile_definitions(archive_tests PRIVATE KRKR2_ARCHIVE_TEST_XP3)
endif()
target_include_directories(archive_tests PRIVATE ${KRKR2CORE_PATH})
target_compile_definitions(archive_tests PRIVATE KRKR2_ARCHIVE_TEST_DATA="${ARCHIVE_FIXTURES}")
catch_discover_tests(archive_tests)
if(TARGET xp3)
    add_test(NAME xp3_tool_smoke COMMAND ${Python3_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/tools/xp3/smoke_test.py $<TARGET_FILE:xp3>)
endif()
