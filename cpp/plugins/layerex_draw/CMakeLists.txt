cmake_minimum_required(VERSION 3.19)
project(layerExDraw)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PUBLIC
    main.cpp
    LayerExDraw.cpp
    DrawPath.cpp
)

find_package(Libgdiplus REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${Libgdiplus_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE krkr2plugin ${Libgdiplus_LIBRARIES})

set(VCPKG_INSTALL_DIR ${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET})

# glibconfig
# https://invent.kde.org/frameworks/extra-cmake-modules/-/blob/master/find-modules/FindGLIB2.cmake
target_include_directories(${PROJECT_NAME} PUBLIC "${VCPKG_INSTALL_DIR}/lib/glib-2.0/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${VCPKG_INSTALL_DIR}/include/glib-2.0")
