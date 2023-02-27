cmake_minimum_required(VERSION 3.13)

project(ritobin_gui LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ritobin_gui src/main.cpp)

target_link_libraries(ritobin_gui PRIVATE ritobin_lib)
target_include_directories(ritobin_gui PRIVATE deps/)
#RITOBIN_HASHES_copy(ritobin_gui)
