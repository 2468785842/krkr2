vcpkg_check_linkage(ONLY_STATIC_LIBRARY)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO 2468785842/Ciallang
    REF 636edaa928656858689386b32da1275f2f597fe9
    SHA512 e878a37c64b20465e5a5b6541bc8c307f5ceea76533aceb0916fcbe55b1fe7a4985cfc4d20bbfcf99086796cdd41ede966bc31af15ae56c07ae307204a806ee6
    HEAD_REF master
)

vcpkg_find_acquire_program(PYTHON3)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DPython3_EXECUTABLE=${PYTHON3}
        -DCIAL_BUILD_TESTS=OFF
        -DCIAL_BUILD_CLI=OFF
        -DCIAL_BUILD_FORMATTER=OFF
        -DCIAL_BUILD_TOOLS=OFF
        -DCIAL_BUILD_FUZZ=OFF
        -DCIAL_BUILD_BENCH=OFF
        -DCIAL_ENABLE_JIT=OFF
        -DCIAL_WARNINGS_AS_ERRORS=OFF
        -DCIAL_INSTALL_SDK=ON
        -DCIAL_RET_DIAGNOSTICS=ON
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME Ciallang CONFIG_PATH lib/cmake/Ciallang)
vcpkg_copy_pdbs()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include" "${CURRENT_PACKAGES_DIR}/debug/share")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
