function(extract_archive)
    cmake_parse_arguments("arg" "" "ARCHIVE;DESTINATION" "" ${ARGN})
    if (NOT EXISTS "${arg_DESTINATION}")
        file(MAKE_DIRECTORY "${arg_DESTINATION}")
    endif ()
    execute_process(
        COMMAND tar -xzf "${arg_ARCHIVE}" -C "${arg_DESTINATION}" --strip-components=1
        RESULT_VARIABLE TAR_RESULT
        OUTPUT_VARIABLE TAR_OUTPUT
        ERROR_VARIABLE TAR_ERROR
    )

endfunction()

# 定义函数下载并解压归档文件
function(download_and_extract URL FILENAME DESTINATION SHA512)
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
endfunction()

download_and_extract(
    "https://github.com/2468785842/cocos2d-x-3rd-party-libs-bin/archive/refs/heads/v3.tar.gz"
    "cocos2d-x-3rd-party-libs-bin.tar.gz"
    "${SOURCE_PATH}/external"
    "44086c0eb89e3085eaf1e55b326d6b919d839f95bf56868f8eda44de80a526c8910e6587818ee2364c7bc139b032cfdb3bc35c1e2f76d6275b422c74e7bf8614"
)

download_and_extract(
    "https://github.com/cocos2d/cocos2d-console/archive/refs/heads/v3.tar.gz"
    "cocos2d-console.tar.gz"
    "${SOURCE_PATH}/tools/cocos2d-console"
    "f7408b5f41ee4b05a80a7c7717afab4f928e67410cd1c616344d1197591083bf8d6de45efc3bef80c7ab18ab6bc7ad2d9a6d1e05f35d7c00408b53de322489c3"
)

download_and_extract(
    "https://github.com/cocos2d/bindings-generator/archive/refs/heads/v3.tar.gz"
    "bindings-generator.tar.gz"
    "${SOURCE_PATH}/tools/bindings-generator"
    "6a0cc110fa205bee044eb138a5a6df5248da386db3247e0451a60694c66fd3e6fa761e8d28b7b7aca785b11fa0830a10bcf69702dab0bc52550d0fd4da87cd57"
)

download_and_extract(
    "https://github.com/cocos2d/cocos2d-html5/archive/refs/heads/develop.tar.gz"
    "cocos2d-html5.tar.gz"
    "${SOURCE_PATH}/web"
    "b974827acbe4c605552b4e8040936789455ed5faadcd01a8a8abbb05a722ce5b10dceebb5b0fdb6eeda2c446a1c0d55452198467566f70a7582ebe910dfcdfd0"
)

