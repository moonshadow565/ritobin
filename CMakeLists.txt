cmake_minimum_required(VERSION 3.5)

project(ritobin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(ritobin_hashes)
add_subdirectory(ritobin_lib)
add_subdirectory(ritobin_cli)
add_subdirectory(ritobin_gui)
