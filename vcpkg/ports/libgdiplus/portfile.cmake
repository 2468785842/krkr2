set(VCPKG_POLICY_SKIP_ABSOLUTE_PATHS_CHECK enabled)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mono/libgdiplus
    REF "${VERSION}"
    SHA512 fe6a798da6ad194d4e1d3ce2ebb71a43d3f2f3d198bdf053e8a03e861c81c1c838f3d5d60cfde6b1d6f662fb7f9c2192a9acc89c30a65999e841f4ad7b180baf
    PATCHES
        0001-fix-mac.patch
        0001-fix-quartz-api.patch
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/config.h.in" DESTINATION "${SOURCE_PATH}")

set(ENV{PKG_CONFIG} "${CURRENT_HOST_INSTALLED_DIR}/tools/pkgconf/pkgconf${VCPKG_HOST_EXECUTABLE_SUFFIX}")
get_filename_component(PKGCONFIG_PATH "${PKGCONFIG}" DIRECTORY)
vcpkg_add_to_path("${PKGCONFIG_PATH}")

if(VCPKG_TARGET_IS_LINUX)
    set(OPTIONS -DWITH_X11=ON)
elseif (VCPKG_TARGET_IS_ANDROID OR VCPKG_TARGET_IS_OSX)
    set(OPTIONS -DWITH_PANGO=OFF)
endif()

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}" OPTIONS ${OPTIONS})

vcpkg_cmake_install()
vcpkg_fixup_pkgconfig()
vcpkg_copy_pdbs()

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
