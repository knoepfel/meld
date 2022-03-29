find_package(CLI11 REQUIRED)
find_package(TBB REQUIRED COMPONENTS TBB)

add_subdirectory(core)

add_library(run_sand SHARED run_sand.cc)
target_link_libraries(run_sand PRIVATE TBB::TBB sand_core)
target_include_directories(run_sand PRIVATE ${PROJECT_SOURCE_DIR})

configure_file(sand.cc.in sand.cc @ONLY)
add_executable(sand sand.cc)
target_link_libraries(sand PRIVATE CLI11::CLI11 run_sand)
target_include_directories(sand PRIVATE ${PROJECT_SOURCE_DIR})
