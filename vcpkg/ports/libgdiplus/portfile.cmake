set(VCPKG_POLICY_ALLOW_RESTRICTED_HEADERS enabled)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mono/libgdiplus
    REF "${VERSION}"
    SHA512 fe6a798da6ad194d4e1d3ce2ebb71a43d3f2f3d198bdf053e8a03e861c81c1c838f3d5d60cfde6b1d6f662fb7f9c2192a9acc89c30a65999e841f4ad7b180baf
    PATCHES
        fix-deps.patch
)

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

file(COPY "${SOURCE_PATH}/src/" DESTINATION "${CURRENT_PACKAGES_DIR}/include/libgdiplus" FILES_MATCHING PATTERN "*.h")
file(COPY "${SOURCE_PATH}/src/" DESTINATION "${CURRENT_PACKAGES_DIR}/include/libgdiplus" FILES_MATCHING PATTERN "*.inc")
if (NOT DEFINED VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    file(COPY "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-dbg/config.h" DESTINATION "${CURRENT_PACKAGES_DIR}/include/libgdiplus")
else()
    file(COPY "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rel/config.h" DESTINATION "${CURRENT_PACKAGES_DIR}/include/libgdiplus")
endif ()
# Handle dependencies
set(Libgdiplus_PKGCONFIG_MODULES "zlib libjpeg libpng cairo cairo-script-interpreter glib-2.0 libtiff-4 libexif")
x_vcpkg_pkgconfig_get_modules(PREFIX Libgdiplus_PKGCONFIG MODULES ${Libgdiplus_PKGCONFIG_MODULES} LIBS)

function(append_dependencies_from_libs out)
    cmake_parse_arguments(PARSE_ARGV 1 "arg" "" "LIBS" "")
    string(REGEX REPLACE "[ ]+" ";" contents "${arg_LIBS}")
    list(FILTER contents EXCLUDE REGEX "^-F.+")
    list(FILTER contents EXCLUDE REGEX "^-framework$")
    list(FILTER contents EXCLUDE REGEX "^-L.+")
    list(FILTER contents EXCLUDE REGEX "^-libpath:.+")
    list(TRANSFORM contents REPLACE "^-Wl,-framework," "-l")
    list(FILTER contents EXCLUDE REGEX "^-Wl,.+")
    list(TRANSFORM contents REPLACE "^-l" "")
    if (VCPKG_TARGET_IS_WINDOWS)
        list(TRANSFORM contents TOLOWER)
    endif ()
    if (contents)
        list(APPEND "${out}" "${contents}")
        set("${out}" "${${out}}" PARENT_SCOPE)
    endif ()
endfunction()

append_dependencies_from_libs(Libgdiplus_DEPENDENCIES_RELEASE LIBS "${Libgdiplus_PKGCONFIG_LIBS_RELEASE}")
append_dependencies_from_libs(Libgdiplus_DEPENDENCIES_DEBUG LIBS "${Libgdiplus_PKGCONFIG_LIBS_DEBUG}")

# must remove duplicates from the front to respect link order so reverse first
list(REVERSE Libgdiplus_DEPENDENCIES_RELEASE)
list(REVERSE Libgdiplus_DEPENDENCIES_DEBUG)
list(REMOVE_DUPLICATES Libgdiplus_DEPENDENCIES_RELEASE)
list(REMOVE_DUPLICATES Libgdiplus_DEPENDENCIES_DEBUG)
list(REVERSE Libgdiplus_DEPENDENCIES_RELEASE)
list(REVERSE Libgdiplus_DEPENDENCIES_DEBUG)

message(STATUS "Dependencies (release): ${Libgdiplus_DEPENDENCIES_RELEASE}")
message(STATUS "Dependencies (debug):   ${Libgdiplus_DEPENDENCIES_DEBUG}")

configure_file("${CMAKE_CURRENT_LIST_DIR}/FindLibgdiplus.cmake.in" "${CURRENT_PACKAGES_DIR}/share/${PORT}/FindLibgdiplus.cmake" @ONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/vcpkg-cmake-wrapper.cmake" "${CURRENT_PACKAGES_DIR}/share/${PORT}/vcpkg-cmake-wrapper.cmake" @ONLY)
file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
