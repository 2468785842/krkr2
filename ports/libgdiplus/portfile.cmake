set(VCPKG_POLICY_ALLOW_RESTRICTED_HEADERS enabled)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mono/libgdiplus
    REF "${VERSION}"
    SHA512 fe6a798da6ad194d4e1d3ce2ebb71a43d3f2f3d198bdf053e8a03e861c81c1c838f3d5d60cfde6b1d6f662fb7f9c2192a9acc89c30a65999e841f4ad7b180baf
)

file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/configure.ac" "${SOURCE_PATH}/configure.ac" ONLY_IF_DIFFERENT)

vcpkg_configure_make(
    AUTOCONFIG
    DISABLE_VERBOSE_FLAGS
    SOURCE_PATH ${SOURCE_PATH}
    OPTIONS
        --without-x11
        --without-libgif
    OPTIONS_RELEASE
    OPTIONS_DEBUG
        --enable-debug
)

vcpkg_install_make()
vcpkg_fixup_pkgconfig()
vcpkg_copy_pdbs()

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

file(COPY "${SOURCE_PATH}/src/" DESTINATION "${CURRENT_PACKAGES_DIR}/include" FILES_MATCHING PATTERN "*.h")
file(COPY "${SOURCE_PATH}/src/" DESTINATION "${CURRENT_PACKAGES_DIR}/include" FILES_MATCHING PATTERN "*.inc")

configure_file("${CMAKE_CURRENT_LIST_DIR}/FindLibgdiplus.cmake.in" "${CURRENT_PACKAGES_DIR}/share/${PORT}/FindLibgdiplus.cmake" @ONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/vcpkg-cmake-wrapper.cmake" "${CURRENT_PACKAGES_DIR}/share/${PORT}/vcpkg-cmake-wrapper.cmake" @ONLY)
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
