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
        patch/0001-add-cstdint-header.patch
        patch/fix-iconv-cast.patch
        patch/fix-mac-audio-build.patch
        patch/fix-mac-glew.patch
        patch/fix-mac-glfw3.patch
        patch/fix-unzip.patch
        patch/fix-chipmunk-Hasty.patch
        patch/fix-win64.patch
        patch/fix-bullet-spell.patch
        patch/fix-chipmunk.patch
)

file(COPY "${CMAKE_CURRENT_LIST_DIR}/cocos2dx-config.cmake.in" DESTINATION "${SOURCE_PATH}")

file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/CMakeLists.txt" "${SOURCE_PATH}/CMakeLists.txt" ONLY_IF_DIFFERENT)
file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/cocos/CMakeLists.txt" "${SOURCE_PATH}/cocos/CMakeLists.txt" ONLY_IF_DIFFERENT)
file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/cocos/network/CMakeLists.txt" "${SOURCE_PATH}/cocos/network/CMakeLists.txt" ONLY_IF_DIFFERENT)

file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/cmake/Modules/CocosBuildHelpers.cmake" "${SOURCE_PATH}/cmake/Modules/CocosBuildHelpers.cmake" ONLY_IF_DIFFERENT)
file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/cmake/Modules/CocosConfigDepend.cmake" "${SOURCE_PATH}/cmake/Modules/CocosConfigDepend.cmake" ONLY_IF_DIFFERENT)

include("${CMAKE_CURRENT_LIST_DIR}/DownloadDeps.cmake")

file(COPY_FILE "${CMAKE_CURRENT_LIST_DIR}/patch/cocos2d-x/external/CMakeLists.txt" "${SOURCE_PATH}/external/CMakeLists.txt" ONLY_IF_DIFFERENT)

# Map vcpkg manifest features -> cocos2d-x CMake options.
macro(cocos2dx_set_feature FEATURE_NAME)
    if("${FEATURE_NAME}" IN_LIST FEATURES)
        set("${ARGV1}" ON)
    else()
        set("${ARGV1}" OFF)
    endif()
endmacro()

cocos2dx_set_feature(ui BUILD_EXT_FREETYPE2)
cocos2dx_set_feature(ui BUILD_EXT_JPEG)
cocos2dx_set_feature(ui BUILD_EXT_WEBP)
cocos2dx_set_feature(ui BUILD_EXT_TIFF)

cocos2dx_set_feature(archive BUILD_EXT_ZLIB)
if(BUILD_EXT_ZLIB)
    set(BUILD_EXT_UNZIP ON)
else()
    set(BUILD_EXT_UNZIP OFF)
endif()

cocos2dx_set_feature(network BUILD_EXT_CURL)
cocos2dx_set_feature(network BUILD_EXT_OPENSSL)

cocos2dx_set_feature(extensions BUILD_EXTENSIONS)
cocos2dx_set_feature(cocostudio BUILD_EDITOR_COCOSTUDIO)
cocos2dx_set_feature(spine BUILD_EDITOR_SPINE)
cocos2dx_set_feature(cocosbuilder BUILD_EDITOR_COCOSBUILDER)

cocos2dx_set_feature(physics2d BUILD_EXT_BOX2D)
cocos2dx_set_feature(physics2d BUILD_EXT_CHIPMUNK)
cocos2dx_set_feature(physics3d BUILD_EXT_BULLET)
cocos2dx_set_feature(navmesh BUILD_EXT_RECAST)
cocos2dx_set_feature(websockets BUILD_EXT_WEBSOCKETS)
cocos2dx_set_feature(uv BUILD_EXT_UV)

# Linux desktop pulls TIFF via GTK as well; keep ON when desktop+linux.
if("desktop" IN_LIST FEATURES AND VCPKG_TARGET_IS_LINUX)
    set(BUILD_EXT_TIFF ON)
endif()

if("desktop" IN_LIST FEATURES AND VCPKG_TARGET_IS_WINDOWS)
    set(BUILD_EXT_SQLITE ON)
else()
    cocos2dx_set_feature(sqlite BUILD_EXT_SQLITE)
endif()

# Bundled external sources required by the core engine (no vcpkg packages).
set(BUILD_EXT_TINYXML2 ON)
set(BUILD_EXT_XXHASH ON)
set(BUILD_EXT_XXTEA ON)
set(BUILD_EXT_CLIPPER ON)
set(BUILD_EXT_EDTAA3FUNC ON)
set(BUILD_EXT_CONVERTUTF ON)
set(BUILD_EXT_POLY2TRI ON)
set(BUILD_EXT_MD5 ON)

set(COCOS_CMAKE_OPTIONS
    -DBUILD_TESTS=OFF
    -DBUILD_LUA_LIBS=OFF
    -DBUILD_JS_LIBS=OFF
    -DBUILD_EXT_FREETYPE2=${BUILD_EXT_FREETYPE2}
    -DBUILD_EXT_JPEG=${BUILD_EXT_JPEG}
    -DBUILD_EXT_WEBP=${BUILD_EXT_WEBP}
    -DBUILD_EXT_TIFF=${BUILD_EXT_TIFF}
    -DBUILD_EXT_ZLIB=${BUILD_EXT_ZLIB}
    -DBUILD_EXT_UNZIP=${BUILD_EXT_UNZIP}
    -DBUILD_EXT_CURL=${BUILD_EXT_CURL}
    -DBUILD_EXT_OPENSSL=${BUILD_EXT_OPENSSL}
    -DBUILD_EXTENSIONS=${BUILD_EXTENSIONS}
    -DBUILD_EDITOR_COCOSTUDIO=${BUILD_EDITOR_COCOSTUDIO}
    -DBUILD_EDITOR_SPINE=${BUILD_EDITOR_SPINE}
    -DBUILD_EDITOR_COCOSBUILDER=${BUILD_EDITOR_COCOSBUILDER}
    -DBUILD_EXT_BOX2D=${BUILD_EXT_BOX2D}
    -DBUILD_EXT_CHIPMUNK=${BUILD_EXT_CHIPMUNK}
    -DBUILD_EXT_BULLET=${BUILD_EXT_BULLET}
    -DBUILD_EXT_RECAST=${BUILD_EXT_RECAST}
    -DBUILD_EXT_WEBSOCKETS=${BUILD_EXT_WEBSOCKETS}
    -DBUILD_EXT_UV=${BUILD_EXT_UV}
    -DBUILD_EXT_TIFF=${BUILD_EXT_TIFF}
    -DBUILD_EXT_SQLITE=${BUILD_EXT_SQLITE}
    -DBUILD_EXT_TINYXML2=${BUILD_EXT_TINYXML2}
    -DBUILD_EXT_XXHASH=${BUILD_EXT_XXHASH}
    -DBUILD_EXT_XXTEA=${BUILD_EXT_XXTEA}
    -DBUILD_EXT_CLIPPER=${BUILD_EXT_CLIPPER}
    -DBUILD_EXT_EDTAA3FUNC=${BUILD_EXT_EDTAA3FUNC}
    -DBUILD_EXT_CONVERTUTF=${BUILD_EXT_CONVERTUTF}
    -DBUILD_EXT_POLY2TRI=${BUILD_EXT_POLY2TRI}
    -DBUILD_EXT_MD5=${BUILD_EXT_MD5}
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${COCOS_CMAKE_OPTIONS}
)

vcpkg_cmake_install()

vcpkg_copy_pdbs()

vcpkg_cmake_config_fixup()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(GLOB LICENSE_FILES "${SOURCE_PATH}/licenses/*")
vcpkg_install_copyright(FILE_LIST ${LICENSE_FILES})
