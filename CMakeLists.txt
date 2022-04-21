cmake_minimum_required(VERSION 3.22)

project(meld VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Werror -pedantic)

find_package(Boost REQUIRED)
find_package(TBB REQUIRED)

add_subdirectory(meld)

include(CTest)
add_subdirectory(test)
