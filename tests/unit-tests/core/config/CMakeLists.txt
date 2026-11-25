add_executable(config_format_tests ConfigFormatTest.cpp)
target_link_libraries(config_format_tests PRIVATE Catch2::Catch2WithMain core_config_format)
target_compile_definitions(config_format_tests PRIVATE
    KRKR_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets"
    KRKR_LEGACY_LOCALE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures/legacy-locales")
catch_discover_tests(config_format_tests)

add_executable(host_asset_tests LocalAssetReaderTest.cpp)
target_link_libraries(host_asset_tests PRIVATE Catch2::Catch2WithMain core_host_assets)
catch_discover_tests(host_asset_tests)

add_executable(locale_config_tests ../environ/LocaleConfigTest.cpp)
target_link_libraries(locale_config_tests PRIVATE Catch2::Catch2WithMain core_config)

add_executable(config_manager_tests ConfigManagerTest.cpp)
target_link_libraries(config_manager_tests PRIVATE Catch2::Catch2WithMain core_config)
catch_discover_tests(config_manager_tests)
catch_discover_tests(locale_config_tests)

if(KRKR2_TEST_SCOPE STREQUAL "full" AND NOT KRKR2_SDL3_ONLY)
    find_package(cocos2dx CONFIG REQUIRED)
    add_executable(config_format_legacy_tests ConfigFormatLegacyTest.cpp HostAssetLegacyTest.cpp
        ${CMAKE_SOURCE_DIR}/cpp/core/environ/cocos2d/CocosPackagedAsset.cpp)
    target_compile_definitions(config_format_legacy_tests PRIVATE CC_STATIC
        KRKR_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets"
        KRKR_LEGACY_LOCALE_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures/legacy-locales")
    target_link_libraries(config_format_legacy_tests PRIVATE Catch2::Catch2WithMain
        core_config_format core_host_assets cocos2dx::cocos2d)
    catch_discover_tests(config_format_legacy_tests)
endif()
