cmake_minimum_required(VERSION 3.5)

project(ritobin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_STANDARD_LIBRARIES "kernel32.lib User32.lib Shell32.lib")
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

add_subdirectory(ritobin_hashes)
add_subdirectory(ritobin_lib)
add_subdirectory(ritobin_cli)
add_subdirectory(ritobin_gui)
