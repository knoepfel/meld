cmake_minimum_required(VERSION 3.22)

project(meld VERSION 0.0.1 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE "RelWithDebInfo")
add_compile_options(-Wall -Werror -Wunused -Wunused-parameter -pedantic)

# GCC 14.1 issues many false positives re. array-bounds and stringop-overflow
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "14.1")
  add_compile_options(-Wno-array-bounds -Wno-stringop-overflow)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)

# add_compile_options(-fprofile-instr-generate -fcoverage-mapping)

list(PREPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/Modules)

find_library(BACKTRACE_LIB backtrace REQUIRED)
find_path(BACKTRACE_INC backtrace.h REQUIRED)
find_package(Boost REQUIRED COMPONENTS json program_options stacktrace_basic)
find_package(TBB REQUIRED)
find_package(fmt REQUIRED)
find_package(jsonnet REQUIRED)
find_package(spdlog REQUIRED)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

add_subdirectory(meld)

include(CTest)
add_subdirectory(test)
