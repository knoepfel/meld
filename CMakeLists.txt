cmake_minimum_required(VERSION 3.22)

project(meld VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "Debug")
add_compile_options(-Wall -Werror -pedantic)

find_package(Boost REQUIRED COMPONENTS json program_options)
find_package(TBB REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

add_subdirectory(meld)

include(CTest)
add_subdirectory(test)
