
set(INPUT_SOURCES InputStateTest.cpp InputModifiersTest.cpp PointerCoordinatesTest.cpp InputCancellationTest.cpp TouchButtonTest.cpp TouchButtonPeer.cpp TextInputTest.cpp)
add_executable(input_state_tests ${INPUT_SOURCES})
target_link_libraries(input_state_tests PRIVATE Catch2::Catch2WithMain core_input)
catch_discover_tests(input_state_tests)

# Compile the backend-independent hook source without the legacy event host.
set(FRAME_HOOK_SOURCES PostUpdateTest.cpp PostUpdateRegistration.cpp
    ${CMAKE_SOURCE_DIR}/cpp/core/event/PostUpdateEvent.cpp
    ${CMAKE_SOURCE_DIR}/cpp/core/event/FrameStep.cpp)
add_executable(frame_hook_tests ${FRAME_HOOK_SOURCES})
target_include_directories(frame_hook_tests PRIVATE ${CMAKE_SOURCE_DIR}/cpp/core/event)
target_link_libraries(frame_hook_tests PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(frame_hook_tests)

add_executable(main_thread_message_tests MainThreadMessageQueueTest.cpp
    ${CMAKE_SOURCE_DIR}/cpp/core/event/MainThreadMessageQueue.cpp
    ${CMAKE_SOURCE_DIR}/cpp/core/base/impl/NativeEventQueue.cpp)
target_include_directories(main_thread_message_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/cpp/core/event ${CMAKE_SOURCE_DIR}/cpp/core/base/impl)
find_package(Threads REQUIRED)
target_link_libraries(main_thread_message_tests PRIVATE Catch2::Catch2WithMain Threads::Threads)
catch_discover_tests(main_thread_message_tests)

add_executable(texture_presentation_tests TexturePresentationTest.cpp RenderSettingsTest.cpp)
target_include_directories(texture_presentation_tests PRIVATE ${CMAKE_SOURCE_DIR}/cpp/core/visual)
target_link_libraries(texture_presentation_tests PRIVATE Catch2::Catch2WithMain)
catch_discover_tests(texture_presentation_tests)

if(KRKR2_BUILD_APP)
    add_executable(pvrtc_decoder_tests PVRTCDecoderTest.cpp
        ${CMAKE_SOURCE_DIR}/cpp/core/visual/PVRTCDecoder.cpp)
    target_include_directories(pvrtc_decoder_tests PRIVATE ${CMAKE_SOURCE_DIR}/cpp/core/visual)
    target_link_libraries(pvrtc_decoder_tests PRIVATE Catch2::Catch2WithMain)
    catch_discover_tests(pvrtc_decoder_tests)
endif()

if(TARGET krkr2)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME windowex-lifetime-runtime
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/game-smoke/windowex_lifetime_repro.py
            --engine $<TARGET_FILE:krkr2>
            --output-dir ${CMAKE_BINARY_DIR}/windowex-lifetime)
    set_tests_properties(windowex-lifetime-runtime PROPERTIES TIMEOUT 35 LABELS "integration;plugin")
endif()
