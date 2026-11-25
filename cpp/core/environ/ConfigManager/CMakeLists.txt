cmake_minimum_required(VERSION 3.19)
project(core_config_format LANGUAGES CXX)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)
add_library(${PROJECT_NAME} STATIC ConfigFormat.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json pugixml::pugixml)

add_library(core_config STATIC GlobalConfigManager.cpp IndividualConfigManager.cpp
    LocaleConfigManager.cpp PreferenceStorage.cpp LocalHostServices.cpp)
target_include_directories(core_config PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(core_config PUBLIC core_config_format core_host_assets)
find_package(Threads REQUIRED)
target_link_libraries(core_config PRIVATE Threads::Threads)
