find_package(CLI11 REQUIRED)

add_subdirectory(core)

configure_file(version.cpp.in version.cpp @ONLY)

add_library(run_sand SHARED run_sand.cpp version.cpp)
target_link_libraries(run_sand PRIVATE Boost::boost TBB::tbb sand::core)

add_executable(sand sand.cpp)
target_link_libraries(sand PRIVATE Boost::boost CLI11::CLI11 run_sand sand::core)
target_link_options(sand PRIVATE -Wl,-export_dynamic)
