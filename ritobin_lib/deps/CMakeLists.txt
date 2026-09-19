cmake_minimum_required(VERSION 3.20..4.0)

project(deps LANGUAGES C CXX)

include(FetchContent)

cmake_policy(SET CMP0169 OLD)

FetchContent_Declare(
    xxhash
    GIT_REPOSITORY https://github.com/Cyan4973/xxHash.git
    GIT_TAG        35b0373c697b5f160d3db26b1cbb45a0d5ba788c # v0.8.1
)
FetchContent_GetProperties(xxhash)
if(NOT xxhash_POPULATED)
    FetchContent_Populate(xxhash)
    add_library(xxhash STATIC ${xxhash_SOURCE_DIR}/xxhash.c)
    target_include_directories(xxhash PUBLIC ${xxhash_SOURCE_DIR})
    target_compile_definitions(xxhash INTERFACE -DXXH_STATIC_LINKING_ONLY)
    target_compile_definitions(xxhash PRIVATE -DXXH_CPU_LITTLE_ENDIAN)
endif()


FetchContent_Declare(
    zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG        63779c798237346c2b245c546c40b72a5a5913fe # v1.5.5
)
FetchContent_GetProperties(zstd)
if(NOT zstd_POPULATED)
    FetchContent_Populate(zstd)
    file(GLOB zstd_SRCS
        ${zstd_SOURCE_DIR}/lib/common/*.c
        ${zstd_SOURCE_DIR}/lib/compress/*.c
        ${zstd_SOURCE_DIR}/lib/decompress/*.c
    )
    add_library(zstd STATIC ${zstd_SRCS})
    target_include_directories(zstd PUBLIC ${zstd_SOURCE_DIR}/lib)
    target_include_directories(zstd PRIVATE ${zstd_SOURCE_DIR}/lib/common)
    target_compile_definitions(zstd INTERFACE -DZSTD_STATIC_LINKING_ONLY)
    target_compile_definitions(zstd PRIVATE -DZSTD_DISABLE_ASM)
    if(MSVC)
        target_compile_options(zstd PRIVATE /wd4267)
    endif()
endif()

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        55f93686c01528224f448c19128836e7df245f72 # 3.12.0
)
FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_library(json INTERFACE)
    target_include_directories(json INTERFACE ${json_SOURCE_DIR}/single_include/nlohmann)
endif()
