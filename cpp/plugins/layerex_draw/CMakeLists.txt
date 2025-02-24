cmake_minimum_required(VERSION 3.19)
project(layerExDraw)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PUBLIC
    main.cpp
    LayerExDraw.cpp
    DrawPath.cpp
)

find_package(libgdiplus CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE krkr2plugin libgdiplus::libgdiplus)

# glibconfig
# https://invent.kde.org/frameworks/extra-cmake-modules/-/blob/master/find-modules/FindGLIB2.cmake
target_include_directories(${PROJECT_NAME} PUBLIC "${VCPKG_INSTALL_DIR}/lib/glib-2.0/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${VCPKG_INSTALL_DIR}/include/glib-2.0")
