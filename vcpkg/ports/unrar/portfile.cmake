vcpkg_download_distfile(
    archive
    URLS https://www.rarlab.com/rar/unrarsrc-${VERSION}.tar.gz
    FILENAME unrarsrc-${VERSION}.tar.gz
    SHA512 2c50d1f58f5189e59dad36eb25aa50a34572f583242e624846c9791c5609e83d4ee76314d785771fe514ec3378749dcb86e4c97a8d2a3ab7b469df49a5c5f412
)

vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${archive}"
    SOURCE_BASE unrarsrc-${VERSION}
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/unrar-config.cmake.in" DESTINATION "${SOURCE_PATH}")

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/license.txt")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
