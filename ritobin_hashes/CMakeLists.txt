cmake_minimum_required(VERSION 3.11)

project(ritobin_hashes)

set(RITOBIN_HASHES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../LeagueHashes)
file(GLOB RITOBIN_HASHES ${RITOBIN_HASHES_DIR}/hashes.*.txt*)

add_custom_target(ritobin_hashes SOURCES ${RITOBIN_HASHES} DEPENDS ${RITOBIN_HASHES})

function(RITOBIN_HASHES_copy tgt)
    get_target_property(RITOBIN_HASHES_FILES ritobin_hashes SOURCES)
    add_custom_command(TARGET ${tgt} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${tgt}>/hashes
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RITOBIN_HASHES_FILES} $<TARGET_FILE_DIR:${tgt}>/hashes/)
endfunction()

