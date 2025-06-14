vcpkg_download_distfile(
    archive
    URLS https://bellard.org/bpg/libbpg-${VERSION}.tar.gz
    FILENAME libbpg-${VERSION}.tar.gz
    SHA512 2d5f7a035033d8b969c2765a4f1368e1a2c8c5ace12aec5d449ca5cc4ef5eb89431a75f2d6d03fd7117b548801b5554503411f499f497da69ff33ca6e714553d
)

vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${archive}"
    SOURCE_BASE libbpg-${VERSION}
    PATCHES
        remove-msvc-avpriv-linker.patch
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/libbpg-config.cmake.in" DESTINATION "${SOURCE_PATH}")

if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_replace_string("${SOURCE_PATH}/config.h" "define HAVE_ATOMICS_GCC 1" "define HAVE_ATOMICS_GCC 0")
    vcpkg_replace_string("${SOURCE_PATH}/config.h" "define HAVE_ATOMICS_WIN32 0" "define HAVE_ATOMICS_WIN32 1")
endif()

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/x265/COPYING")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
