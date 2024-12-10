set(COCOS2DX_PREV_MODULE_PATH ${CMAKE_MODULE_PATH})
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR})

cmake_policy(SET CMP0012 NEW)

set(z_vcpkg_using_vcpkg_find_cocos2dx OFF)

_find_package(${ARGS})

if(NOT z_vcpkg_using_vcpkg_find_cocos2dx)

  include(SelectLibraryConfigurations)

endif(NOT z_vcpkg_using_vcpkg_find_cocos2dx)
unset(z_vcpkg_using_vcpkg_find_cocos2dx)

set(COCOS2DX_LIBRARY ${COCOS2DX_LIBRARIES})
set(CMAKE_MODULE_PATH ${COCOS2DX_PREV_MODULE_PATH})