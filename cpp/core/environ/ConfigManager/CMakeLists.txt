cmake_minimum_required(VERSION 3.19)
project(core_config_xml LANGUAGES CXX)

find_package(LibXml2 CONFIG REQUIRED)
add_library(${PROJECT_NAME} STATIC ConfigXml.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE LibXml2::LibXml2)

add_library(core_config STATIC GlobalConfigManager.cpp IndividualConfigManager.cpp
    LocaleConfigManager.cpp PreferenceStorage.cpp LocalHostServices.cpp)
target_include_directories(core_config PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(core_config PUBLIC core_config_xml core_host_assets)
find_package(Threads REQUIRED)
target_link_libraries(core_config PRIVATE Threads::Threads)
