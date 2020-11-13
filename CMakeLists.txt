cmake_minimum_required(VERSION 3.5)

project(ritobin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_STANDARD_LIBRARIES "kernel32.lib User32.lib")
endif()

add_library(ritobin_lib STATIC
    src/ritobin/bin_hash.hpp
    src/ritobin/bin_hash.cpp
    src/ritobin/bin_io.hpp
    src/ritobin/bin_io_bincompat.cpp
    src/ritobin/bin_io_binary_read.cpp
    src/ritobin/bin_io_binary_write.cpp
    src/ritobin/bin_io_text_read.cpp
    src/ritobin/bin_io_text_write.cpp
    src/ritobin/bin_morph.hpp
    src/ritobin/bin_morph_value.cpp
    src/ritobin/bin_morph_type_key.cpp
    src/ritobin/bin_morph_type_value.cpp
    src/ritobin/bin_numconv.hpp
    src/ritobin/bin_numconv.cpp
    src/ritobin/bin_types.hpp
    src/ritobin/bin_types_helper.hpp
    src/ritobin/bin_unhash.hpp
    src/ritobin/bin_unhash.cpp
)
target_include_directories(ritobin_lib PUBLIC src/)

add_executable(ritobin src/main.cpp src/argparse.hpp)
target_link_libraries(ritobin PRIVATE ritobin_lib)
