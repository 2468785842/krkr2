function(extract_archive)
    cmake_parse_arguments("arg" "" "ARCHIVE;DESTINATION" "" ${ARGN})
    if(NOT EXISTS "${arg_DESTINATION}")
        file(MAKE_DIRECTORY "${arg_DESTINATION}")
    endif()
    execute_process(
        COMMAND tar -xzf "${arg_ARCHIVE}" -C "${arg_DESTINATION}" --strip-components=1
        RESULT_VARIABLE TAR_RESULT
        OUTPUT_VARIABLE TAR_OUTPUT
        ERROR_VARIABLE TAR_ERROR
    )

endfunction()

# 定义函数判断文件夹是否为空
function(is_directory_empty TARGET_DIR RESULT_VAR)
    set(${RESULT_VAR} FALSE PARENT_SCOPE)

    # 检查文件夹是否存在
    if(EXISTS "${TARGET_DIR}")
        # 获取文件夹中的内容
        file(GLOB FILES_IN_DIR "${TARGET_DIR}/*")
        
        # 判断列表长度
        list(LENGTH FILES_IN_DIR FILE_COUNT)
        if(FILE_COUNT EQUAL 0)
            set(${RESULT_VAR} TRUE PARENT_SCOPE)
        endif()
    else()
        message(WARNING "The folder '${TARGET_DIR}' does not exist.")
    endif()
endfunction()

# 定义函数下载并解压归档文件
function(download_and_extract URL FILENAME DESTINATION SHA512)
    is_directory_empty("${DESTINATION}" IS_EMPTY)
    if(IS_EMPTY)
        vcpkg_download_distfile(
            ARCHIVE
            URLS ${URL}
            FILENAME ${FILENAME}
            SHA512 ${SHA512}
        )

        extract_archive(
            ARCHIVE "${ARCHIVE}"
            DESTINATION "${DESTINATION}"
        )
    endif()
endfunction()

if((EXISTS "${DOWNLOADS}/v3-deps-158.zip") AND (NOT EXISTS "${SOURCE_PATH}/v3-deps-158.zip"))
    message(STATUS "Using cached v3-deps-158.zip")
    file(COPY_FILE "${DOWNLOADS}/v3-deps-158.zip" "${SOURCE_PATH}/v3-deps-158.zip")
endif()

vcpkg_find_acquire_program(PYTHON2)
vcpkg_execute_required_process(
    COMMAND "${PYTHON2}" download-deps.py -r no
    WORKING_DIRECTORY "${SOURCE_PATH}"
    LOGNAME build-${TARGET_TRIPLET}
)

if(NOT EXISTS "${DOWNLOADS}/v3-deps-158.zip")
    file(RENAME "${SOURCE_PATH}/v3-deps-158.zip" "${DOWNLOADS}/v3-deps-158.zip")
endif()

file(REMOVE "${SOURCE_PATH}/v3-deps-158.zip")

download_and_extract(
    "https://github.com/cocos2d/cocos2d-console/archive/refs/heads/v3.tar.gz"
    "cocos2d-console.tar.gz"
    "${SOURCE_PATH}/tools/cocos2d-console"
    "f7408b5f41ee4b05a80a7c7717afab4f928e67410cd1c616344d1197591083bf8d6de45efc3bef80c7ab18ab6bc7ad2d9a6d1e05f35d7c00408b53de322489c3"
)

# 使用封装的函数处理 bindings-generator
download_and_extract(
    "https://github.com/cocos2d/bindings-generator/archive/refs/heads/v3.tar.gz"
    "bindings-generator.tar.gz"
    "${SOURCE_PATH}/tools/bindings-generator"
    "6a0cc110fa205bee044eb138a5a6df5248da386db3247e0451a60694c66fd3e6fa761e8d28b7b7aca785b11fa0830a10bcf69702dab0bc52550d0fd4da87cd57"
)

# 使用封装的函数处理 cocos2dx-html5
download_and_extract(
    "https://github.com/cocos2d/cocos2d-html5/archive/refs/heads/develop.tar.gz"
    "cocos2d-html5.tar.gz"
    "${SOURCE_PATH}/web"
    "b974827acbe4c605552b4e8040936789455ed5faadcd01a8a8abbb05a722ce5b10dceebb5b0fdb6eeda2c446a1c0d55452198467566f70a7582ebe910dfcdfd0"
)

