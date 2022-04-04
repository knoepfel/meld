find_package(CLI11 REQUIRED)
find_package(TBB REQUIRED)

add_subdirectory(core)

configure_file(version.cpp.in version.cpp @ONLY)

add_library(run_sand SHARED run_sand.cpp version.cpp)
target_link_libraries(run_sand PRIVATE Boost::boost TBB::tbb sand_core)
target_include_directories(run_sand PRIVATE ${PROJECT_SOURCE_DIR})

add_executable(sand sand.cpp)
target_link_libraries(sand PRIVATE CLI11::CLI11 run_sand)
target_include_directories(sand PRIVATE ${PROJECT_SOURCE_DIR})
