vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mono/libgdiplus
    REF "${VERSION}"
    SHA512 c51f2702eb5eee0b7975ddc5840888d11cc0d3ea0e6c3c49afca42ef4ca90064b9ece30c447948647c950a1af36f780c79b7d07b304ec3a855cbf3da2371e94d
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/config.h.in" DESTINATION "${SOURCE_PATH}")
file(COPY "${CMAKE_CURRENT_LIST_DIR}/libgdiplus.pc.in" DESTINATION "${SOURCE_PATH}")

set(ENV{PKG_CONFIG} "${CURRENT_HOST_INSTALLED_DIR}/tools/pkgconf/pkgconf${VCPKG_HOST_EXECUTABLE_SUFFIX}")
get_filename_component(PKGCONFIG_PATH "${PKGCONFIG}" DIRECTORY)
vcpkg_add_to_path("${PKGCONFIG_PATH}")

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}")

vcpkg_cmake_install()
vcpkg_fixup_pkgconfig()
vcpkg_copy_pdbs()

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
