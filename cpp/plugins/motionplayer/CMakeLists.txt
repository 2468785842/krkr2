cmake_minimum_required(VERSION 3.19)
project(motionplayer)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME}
    PRIVATE
        EmotePlayer.cpp
        Player.cpp
        ResourceManager.cpp
    PUBLIC
        main.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE krkr2plugin)
