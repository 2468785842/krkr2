cmake_minimum_required(VERSION 3.19)
project(core_visual_module LANGUAGES CXX)

set(VISUAL_PATH ${CMAKE_CURRENT_LIST_DIR})
set(VISUAL_SOURCE_FILES
    ${VISUAL_PATH}/LoadWEBP.cpp
    ${VISUAL_PATH}/TransIntf.cpp
    ${VISUAL_PATH}/MenuItemIntf.cpp
    ${VISUAL_PATH}/SaveTLG5.cpp
    ${VISUAL_PATH}/FontSystem.cpp
    ${VISUAL_PATH}/tvpps.inc
    ${VISUAL_PATH}/LoadPVRv3.cpp
    ${VISUAL_PATH}/RectItf.cpp
    ${VISUAL_PATH}/VideoOvlIntf.cpp
    ${VISUAL_PATH}/LayerBitmapIntf.cpp
    ${VISUAL_PATH}/FontImpl.cpp
    ${VISUAL_PATH}/RenderManager.cpp
    ${VISUAL_PATH}/LoadJPEG.cpp
    ${VISUAL_PATH}/GraphicsLoaderIntf.cpp
    ${VISUAL_PATH}/SaveTLG6.cpp
    ${VISUAL_PATH}/FreeType.cpp
    ${VISUAL_PATH}/LoadJXR.cpp
    ${VISUAL_PATH}/ImageFunction.cpp
    ${VISUAL_PATH}/tvpgl.cpp
    ${VISUAL_PATH}/LoadTLG.cpp
    ${VISUAL_PATH}/LayerTreeOwnerImpl.cpp
    ${VISUAL_PATH}/GraphicsLoadThread.cpp
    ${VISUAL_PATH}/LayerManager.cpp
    ${VISUAL_PATH}/PrerenderedFont.cpp
    ${VISUAL_PATH}/LoadBPG.cpp
    ${VISUAL_PATH}/WindowIntf.cpp
    ${VISUAL_PATH}/FreeTypeFontRasterizer.cpp
    ${VISUAL_PATH}/LoadPNG.cpp
    ${VISUAL_PATH}/LayerIntf.cpp
    ${VISUAL_PATH}/ComplexRect.cpp
    ${VISUAL_PATH}/CharacterData.cpp
    ${VISUAL_PATH}/BitmapLayerTreeOwner.cpp
    ${VISUAL_PATH}/BitmapIntf.cpp
    ${VISUAL_PATH}/argb.cpp

    ${VISUAL_PATH}/gl/blend_function.cpp
    ${VISUAL_PATH}/gl/ResampleImage.cpp
    ${VISUAL_PATH}/gl/WeightFunctor.cpp

    ${VISUAL_PATH}/ogl/astcrt.cpp
    ${VISUAL_PATH}/ogl/etcpak.cpp
    ${VISUAL_PATH}/ogl/imagepacker.cpp
    ${VISUAL_PATH}/ogl/pvrtc.cpp
    ${VISUAL_PATH}/ogl/RenderManager_ogl.cpp

    ${VISUAL_PATH}/impl/BasicDrawDevice.cpp
    ${VISUAL_PATH}/impl/BitmapBitsAlloc.cpp
    ${VISUAL_PATH}/impl/BitmapInfomation.cpp
    ${VISUAL_PATH}/impl/DInputMgn.cpp
    ${VISUAL_PATH}/impl/DrawDevice.cpp
#    ${VISUAL_PATH}/impl/GDIFontRasterizer.cpp
    ${VISUAL_PATH}/impl/GraphicsLoaderImpl.cpp
    ${VISUAL_PATH}/impl/LayerBitmapImpl.cpp
    ${VISUAL_PATH}/impl/LayerImpl.cpp
    ${VISUAL_PATH}/impl/MenuItemImpl.cpp
#    ${VISUAL_PATH}/impl/NativeFreeTypeFace.cpp
    ${VISUAL_PATH}/impl/PassThroughDrawDevice.cpp
    ${VISUAL_PATH}/impl/TVPScreen.cpp
#    ${VISUAL_PATH}/impl/TVPSysFont.cpp
    ${VISUAL_PATH}/impl/VideoOvlImpl.cpp
#    ${VISUAL_PATH}/impl/VSyncTimingThread.cpp
    ${VISUAL_PATH}/impl/WindowImpl.cpp
)

set(VISUAL_HEADERS_DIR
    ${VISUAL_PATH}/
    ${VISUAL_PATH}/gl
    ${VISUAL_PATH}/ogl
    ${VISUAL_PATH}/impl
)

add_library(${PROJECT_NAME} STATIC ${VISUAL_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PUBLIC ${VISUAL_HEADERS_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE tjs2 core_base_module core_environ_module core_sound_module core_utils_module)

target_link_libraries(${PROJECT_NAME} PRIVATE libbpg)

find_package(WebP CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} INTERFACE WebP::webp WebP::webpdecoder WebP::webpdemux)

find_package(cocos2dx CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE
        cocos2dx::cocos2d
        $<$<BOOL:${ANDROID}>:$<LINK_LIBRARY:WHOLE_ARCHIVE,cocos2dx::cpp_android_spec>>
)

find_package(JXR REQUIRED)
target_compile_definitions(${PROJECT_NAME} PRIVATE -D__ANSI__)
target_include_directories(${PROJECT_NAME} PRIVATE ${JXR_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${JXR_LIBRARIES})

find_package(libjpeg-turbo CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC
    $<IF:$<TARGET_EXISTS:libjpeg-turbo::turbojpeg>,libjpeg-turbo::turbojpeg,libjpeg-turbo::turbojpeg-static>
)

set(OpenCV_ROOT "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/opencv4")
find_package(OpenCV REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC
    opencv_imgproc
    opencv_core
)

find_package(lz4 CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC
    lz4::lz4
)
