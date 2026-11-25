# Keep the resource set explicit so packaging and copying use the same list.
file(GLOB_RECURSE KRKR2_ALL_ASSETS CONFIGURE_DEPENDS LIST_DIRECTORIES false
    RELATIVE "${GAME_RES_FOLDER}" "${GAME_RES_FOLDER}/*")
set(KRKR2_SELECTED_ASSETS ${KRKR2_ALL_ASSETS})
set(common_res_files)
foreach(asset IN LISTS KRKR2_SELECTED_ASSETS)
    list(APPEND common_res_files "${GAME_RES_FOLDER}/${asset}")
endforeach()
string(REPLACE ";" "\n" KRKR2_ALL_ASSETS_TEXT "${KRKR2_ALL_ASSETS}")
string(REPLACE ";" "\n" KRKR2_SELECTED_ASSETS_TEXT "${KRKR2_SELECTED_ASSETS}")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/runtime-assets-all.txt"
    CONTENT "${KRKR2_ALL_ASSETS_TEXT}\n")
file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/runtime-assets-selected.txt"
    CONTENT "${KRKR2_SELECTED_ASSETS_TEXT}\n")
