set(Libgdiplus_PREV_MODULE_PATH ${CMAKE_MODULE_PATH})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

cmake_policy(SET CMP0012 NEW)

set(z_vcpkg_using_vcpkg_find_libgdiplus OFF)

_find_package(${ARGS})

# Fixup of variables and targets for (some) vendored find modules
if(NOT z_vcpkg_using_vcpkg_find_libgdiplus)

  include(SelectLibraryConfigurations)

  set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH ON) # Required for CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 3.1 which otherwise ignores CMAKE_PREFIX_PATH

endif(NOT z_vcpkg_using_vcpkg_find_libgdiplus)
unset(z_vcpkg_using_vcpkg_find_libgdiplus)

set(Libgdiplus_LIBRARY ${Libgdiplus_LIBRARIES})
set(CMAKE_MODULE_PATH ${Libgdiplus_PREV_MODULE_PATH})