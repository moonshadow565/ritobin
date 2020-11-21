cmake_minimum_required(VERSION 3.5)

project(ritobin_cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ritobin_cli src/main.cpp)
target_link_libraries(ritobin_cli PRIVATE ritobin_lib)
target_include_directories(ritobin_cli PRIVATE deps/)
RITOBIN_HASHES_copy(ritobin_cli)
