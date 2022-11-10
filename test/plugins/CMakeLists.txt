find_package(HDF5 REQUIRED)
find_package(h5cpp REQUIRED)

add_library(module_t MODULE module_t.cpp)
target_link_libraries(module_t PRIVATE meld::module)

add_library(output_t MODULE output_t.cpp)
target_link_libraries(output_t PRIVATE meld::module)

add_library(source_t MODULE source_t.cpp)
target_link_libraries(source_t PRIVATE Boost::json meld::core)

add_test(NAME job:add_t
         COMMAND meld -c ${CMAKE_CURRENT_SOURCE_DIR}/add.jsonnet -g add.gv)
set_tests_properties(job:add_t PROPERTIES ENVIRONMENT SPDLOG_LEVEL=debug)
