set(COCOS2D_VERSION cocos2d-x-${VERSION})

set(VCPKG_POLICY_SKIP_COPYRIGHT_CHECK enabled)
set(VCPKG_POLICY_ALLOW_EMPTY_FOLDERS enabled)
set(VCPKG_POLICY_SKIP_ABSOLUTE_PATHS_CHECK enabled)

vcpkg_download_distfile(
    ARCHIVE
    URLS https://github.com/cocos2d/cocos2d-x/archive/refs/tags/${COCOS2D_VERSION}.tar.gz
    FILENAME ${COCOS2D_VERSION}.tar.gz
    SHA512 b2d5ac968231892c39a953d82e9791c2182b0dbceca5791647bb2daad258134725386c9eb1d32de148465d88d2d932b29f241af0f5f4b4e6d9d80d9684f531fa
)

vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
    SOURCE_BASE "${COCOS2D_VERSION}"
)

file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/CMakeLists.txt" "${SOURCE_PATH}/CMakeLists.txt" ONLY_IF_DIFFERENT)

file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/android_CCFileUtils-android.h" "${SOURCE_PATH}/cocos/platform/android/CCFileUtils-android.h" ONLY_IF_DIFFERENT)
file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/android_CCFileUtils-android.cpp" "${SOURCE_PATH}/cocos/platform/android/CCFileUtils-android.cpp" ONLY_IF_DIFFERENT)
file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/android_Java_org_cocos2dx_lib_Cocos2dxHelper.h" "${SOURCE_PATH}/cocos/platform/android/jni/Java_org_cocos2dx_lib_Cocos2dxHelper.h" ONLY_IF_DIFFERENT)
file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/android_Java_org_cocos2dx_lib_Cocos2dxHelper.cpp" "${SOURCE_PATH}/cocos/platform/android/jni/Java_org_cocos2dx_lib_Cocos2dxHelper.cpp" ONLY_IF_DIFFERENT)

include("${CMAKE_CURRENT_LIST_DIR}/DownloadDeps.cmake")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTS=OFF
        -DBUILD_JS_LIBS=OFF
        -DBUILD_LUA_LIBS=OFF
)

vcpkg_cmake_install()

vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
configure_file("${CMAKE_CURRENT_LIST_DIR}/FindCOCOS2DX.cmake.in" "${CURRENT_PACKAGES_DIR}/share/${PORT}/FindCOCOS2DX.cmake" @ONLY)
configure_file("${CMAKE_CURRENT_LIST_DIR}/vcpkg-cmake-wrapper.cmake" "${CURRENT_PACKAGES_DIR}/share/${PORT}/vcpkg-cmake-wrapper.cmake" @ONLY)
