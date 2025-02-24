set(COCOS2D_VERSION cocos2d-x-${VERSION})

set(VCPKG_POLICY_ALLOW_RESTRICTED_HEADERS enabled)
set(VCPKG_POLICY_ALLOW_EMPTY_FOLDERS enabled)

vcpkg_download_distfile(
    ARCHIVE
    URLS https://github.com/cocos2d/cocos2d-x/archive/refs/tags/${COCOS2D_VERSION}.tar.gz
    FILENAME ${COCOS2D_VERSION}.tar.gz
    SHA512 b2d5ac968231892c39a953d82e9791c2182b0dbceca5791647bb2daad258134725386c9eb1d32de148465d88d2d932b29f241af0f5f4b4e6d9d80d9684f531fa
)

if(VCPKG_TARGET_IS_LINUX)
    message(WARNING "${PORT} currently requires external library from the system package manager:
    On Ubuntu derivatives:
        sudo apt install libxxf86vm-dev libx11-dev libxmu-dev libglu1-mesa-dev libgl2ps-dev libxi-dev libzip-dev libpng-dev libcurl4-gnutls-dev libfontconfig1-dev libsqlite3-dev libglew-dev libssl-dev libgtk-3-dev binutils")

endif()

vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
    SOURCE_BASE "${COCOS2D_VERSION}"
    PATCHES
        patch/0001-fix-external-api-invoke.patch
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/cocos2dx-config.cmake.in" DESTINATION "${SOURCE_PATH}")

file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/CMakeLists.txt" "${SOURCE_PATH}/CMakeLists.txt" ONLY_IF_DIFFERENT)
file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/cocos/CMakeLists.txt" "${SOURCE_PATH}/cocos/CMakeLists.txt" ONLY_IF_DIFFERENT)

file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/cmake/Modules/CocosBuildHelpers.cmake" "${SOURCE_PATH}/cmake/Modules/CocosBuildHelpers.cmake" ONLY_IF_DIFFERENT)
file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/cmake/Modules/CocosConfigDepend.cmake" "${SOURCE_PATH}/cmake/Modules/CocosConfigDepend.cmake" ONLY_IF_DIFFERENT)

include("${CMAKE_CURRENT_LIST_DIR}/DownloadDeps.cmake")

file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/external/CMakeLists.txt" "${SOURCE_PATH}/external/CMakeLists.txt" ONLY_IF_DIFFERENT)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF
        -DBUILD_JS_LIBS=OFF
        -DBUILD_LUA_LIBS=OFF
)

vcpkg_cmake_install()

vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(GLOB LICENSE_FILES "${SOURCE_PATH}/licenses/*")
vcpkg_install_copyright(FILE_LIST ${LICENSE_FILES})