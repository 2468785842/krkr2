# KrKr2 模块 CMake 辅助（foundation 层与头文件路径）
set(KRKR2_CORE_DIR ${CMAKE_SOURCE_DIR}/cpp/core CACHE INTERNAL "")

set(KRKR2_FOUNDATION_LIBS
    core_log
    tjs2
    core_io
    core_msg
    core_sysinit
    core_script
    core_event
    core_storage_engine
    archive_xp3
    archive_zip
    archive_7z
    archive_tar
)

function(krkr2_target_link_foundation target visibility)
    target_link_libraries(${target} ${visibility} ${KRKR2_FOUNDATION_LIBS})
endfunction()

# base + 已迁出模块头文件路径（替代 base/ 下的兼容转发头）
set(KRKR2_CORE_API_INCLUDE_DIRS
    ${KRKR2_CORE_DIR}/base
    ${KRKR2_CORE_DIR}/base/impl
    ${KRKR2_CORE_DIR}/msg
    ${KRKR2_CORE_DIR}/event
    ${KRKR2_CORE_DIR}/event/impl
    ${KRKR2_CORE_DIR}/sysinit
    ${KRKR2_CORE_DIR}/sysinit/impl
    ${KRKR2_CORE_DIR}/kag
)

function(krkr2_target_include_base_headers target)
    target_include_directories(${target} PRIVATE ${KRKR2_CORE_API_INCLUDE_DIRS})
endfunction()

function(krkr2_target_include_environ_headers target)
    target_include_directories(${target} PRIVATE
        ${KRKR2_CORE_DIR}/environ
        ${KRKR2_CORE_DIR}/environ/impl
        ${KRKR2_CORE_DIR}/environ/ConfigManager
    )
endfunction()
