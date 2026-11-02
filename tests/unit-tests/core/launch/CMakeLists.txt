set(SOURCES LaunchOptionsTest.cpp PreferenceDefaultsTest.cpp)
add_executable(launch_tests ${SOURCES})
target_link_libraries(launch_tests PRIVATE core_launch Catch2::Catch2WithMain)
catch_discover_tests(launch_tests)

if(TARGET krkr2)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    set(cli_mode_args)
    if(NOT KRKR2_USE_COCOS_UI)
        list(APPEND cli_mode_args --require-game)
    endif()
    add_test(NAME runtime_asset_sync
        COMMAND ${Python3_EXECUTABLE} -m unittest discover -s ${CMAKE_SOURCE_DIR}/tools/game-smoke -p test_resources.py -v)
    add_test(NAME launch_cli_smoke
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/game-smoke/cli_smoke.py $<TARGET_FILE:krkr2> ${cli_mode_args})
    set_tests_properties(launch_cli_smoke PROPERTIES TIMEOUT 180)
endif()
