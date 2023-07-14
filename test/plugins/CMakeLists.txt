find_package(HDF5 REQUIRED)
find_package(h5cpp REQUIRED)
find_package(ZLIB REQUIRED)

jsonnet_so_workaround()

add_library(module MODULE module.cpp)
target_link_libraries(module PRIVATE Boost::json meld::module)

add_library(hdf5_output MODULE hdf5_output.cpp)
target_link_libraries(hdf5_output PRIVATE Boost::json meld::module h5cpp HDF5::HDF5 z)

add_library(output MODULE output.cpp)
target_link_libraries(output PRIVATE Boost::json meld::module)

add_library(source MODULE source.cpp)
target_link_libraries(source PRIVATE Boost::json meld::core)

add_test(NAME job:add
         COMMAND meld -c ${CMAKE_CURRENT_SOURCE_DIR}/add.jsonnet -g add.gv)

add_test(NAME job:add_hdf5
  COMMAND meld -c ${CMAKE_CURRENT_SOURCE_DIR}/add_hdf5.jsonnet)

set_tests_properties(job:add job:add_hdf5 PROPERTIES
  ENVIRONMENT SPDLOG_LEVEL=debug
  ENVIRONMENT MELD_PLUGIN_PATH=${CMAKE_CURRENT_BINARY_DIR}
)
