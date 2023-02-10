jsonnet_so_workaround()

add_library(module MODULE module.cpp)
target_link_libraries(module PRIVATE Boost::json meld::module)

add_library(output MODULE output.cpp)
target_link_libraries(output PRIVATE Boost::json meld::module)

add_library(source MODULE source.cpp)
target_link_libraries(source PRIVATE Boost::json meld::core)

add_test(NAME job:add
         COMMAND meld -c ${CMAKE_CURRENT_SOURCE_DIR}/add.jsonnet -g add.gv)

set_tests_properties(job:add
  PROPERTIES
  ENVIRONMENT SPDLOG_LEVEL=debug
  ENVIRONMENT MELD_PLUGIN_PATH=${CMAKE_CURRENT_BINARY_DIR}
)
