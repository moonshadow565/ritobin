cmake_minimum_required(VERSION 3.5)

project(ritobin LANGUAGES CXX)

if (WIN32)
set(CMAKE_CXX_STANDARD_LIBRARIES "kernel32.lib User32.lib")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
endif()

add_library(ritobin STATIC
    src/ritobin/numconv.hpp
    src/ritobin/bin.cpp
    src/ritobin/bin.hpp
    src/ritobin/bin_morph.cpp
    src/ritobin/binary_read.cpp
    src/ritobin/binary_write.cpp
    src/ritobin/text_read.cpp
    src/ritobin/text_write.cpp
    src/ritobin/unhash.cpp
)
target_include_directories(ritobin PUBLIC src/)

add_executable(ritobin2txt src/ritobin2txt.cpp)
target_link_libraries(ritobin2txt PRIVATE ritobin)

add_executable(txt2ritobin src/txt2ritobin.cpp)
target_link_libraries(txt2ritobin PRIVATE ritobin)
