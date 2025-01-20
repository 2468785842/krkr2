vcpkg_download_distfile(
    archive
    URLS https://sourceforge.net/projects/p7zip/files/p7zip/${VERSION}/p7zip_${VERSION}_src_all.tar.bz2
    FILENAME "p7zip_${VERSION}_src_all.tar.bz2"
    SHA512 d2c4d53817f96bb4c7683f42045198d4cd509cfc9c3e2cb85c8d9dc4ab6dfa7496449edeac4e300ecf986a9cbbc90bd8f8feef8156895d94617c04e507add55f
)

vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${archive}"
    SOURCE_BASE "p7zip-${VERSION}"
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/C/" DESTINATION "${SOURCE_PATH}/C")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/p7zip-config.cmake.in" DESTINATION "${SOURCE_PATH}")

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_cmake_install()
vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
