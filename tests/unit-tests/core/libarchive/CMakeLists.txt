find_package(LibArchive REQUIRED)
add_executable(libarchive_dependency_tests LibArchiveDependencyTest.cpp)
# Deliberately do not link the engine or codec libraries here: the imported
# archive target must carry its own static link closure.
target_link_libraries(libarchive_dependency_tests PRIVATE Catch2::Catch2WithMain LibArchive::LibArchive)
target_compile_definitions(libarchive_dependency_tests PRIVATE
    KRKR2_XAR_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixture.xar")
catch_discover_tests(libarchive_dependency_tests)
