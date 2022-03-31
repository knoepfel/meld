cmake_minimum_required(VERSION 3.22)

project(sand VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Werror -pedantic)

find_package(Boost REQUIRED filesystem)

add_subdirectory(sand)

include(CTest)
add_subdirectory(test)
