cmake_minimum_required(VERSION 3.19)
project(motionplayer)

add_library(${PROJECT_NAME} STATIC)

target_sources(${PROJECT_NAME} PUBLIC
    main.cpp
    EmotePlayer.cpp
    Player.cpp
    ResourceManager.cpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE krkr2plugin)
