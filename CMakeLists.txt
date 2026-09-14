cmake_minimum_required(VERSION 3.16)

project(SimpleANNS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenMP REQUIRED COMPONENTS CXX)

add_executable(search src/search.cpp)
add_executable(build_knn src/build_knn.cpp)

target_link_libraries(build_knn PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(search PRIVATE OpenMP::OpenMP_CXX)
