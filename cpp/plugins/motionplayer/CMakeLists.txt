cmake_minimum_required(VERSION 3.19)
project(motionplayer)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE
        main.cpp
        EmotePlayer.cpp
        PlayerCore.cpp
        PlayerFrameProgress.cpp
        PlayerResource.cpp
        PlayerMotionLoad.cpp
        PlayerVariable.cpp
        PlayerTimeline.cpp
        PlayerLayerQuery.cpp
        PlayerDrawDispatch.cpp
        PlayerRender.cpp
        PlayerRenderInternal.cpp
        PlayerRenderItems.cpp
        PlayerRenderExecute.cpp
        PlayerRenderTargets.cpp
        PlayerUpdateLayers.cpp
        PlayerUpdateLayerEval.cpp
        PlayerUpdateGeometry.cpp
        PlayerUpdateChildMotion.cpp
        PlayerUpdateParticles.cpp
        PlayerUpdateAnchor.cpp
        D3DAdaptor.cpp
        ResourceManager.cpp
        SourceCache.cpp
        RuntimeSupport.cpp
        NodeTree.cpp
        MotionNodeBridge.cpp
        MotionTraceWeb.cpp
        PrivateMotionGLL.cpp
        SeparateLayerAdaptor.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE krkr2plugin_api)
