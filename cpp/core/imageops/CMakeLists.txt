add_library(core_imageops STATIC ImageOps.cpp ImageOps_opencv.cpp)
target_compile_features(core_imageops PUBLIC cxx_std_17)
target_include_directories(core_imageops PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)

find_package(OpenCV REQUIRED COMPONENTS core imgproc)
target_include_directories(core_imageops PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(core_imageops PRIVATE opencv_core opencv_imgproc)
