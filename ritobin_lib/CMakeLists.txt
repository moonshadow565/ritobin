cmake_minimum_required(VERSION 3.13)

project(ritobin_lib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ritobin_lib STATIC
    src/ritobin/bin_hash.hpp
    src/ritobin/bin_hash.cpp
    src/ritobin/bin_io.hpp
    src/ritobin/bin_io_dynamic.cpp
    src/ritobin/bin_io_binary_read.cpp
    src/ritobin/bin_io_binary_write.cpp
    src/ritobin/bin_io_json.cpp
    src/ritobin/bin_io_text_read.cpp
    src/ritobin/bin_io_text_write.cpp
    src/ritobin/bin_morph.hpp
    src/ritobin/bin_morph_value.cpp
    src/ritobin/bin_morph_type_key.cpp
    src/ritobin/bin_morph_type_value.cpp
    src/ritobin/bin_numconv.hpp
    src/ritobin/bin_numconv.cpp
    src/ritobin/bin_strconv.hpp
    src/ritobin/bin_strconv.cpp
    src/ritobin/bin_types.hpp
    src/ritobin/bin_types_helper.hpp
    src/ritobin/bin_unhash.hpp
    src/ritobin/bin_unhash.cpp
)

target_include_directories(ritobin_lib PUBLIC src/)
target_include_directories(ritobin_lib PRIVATE deps/)
if (WIN32)
    target_sources(ritobin_lib INTERFACE ../res/utf8.manifest ../res/longpath.manifest)
endif()
