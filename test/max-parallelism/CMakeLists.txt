add_library(check_parallelism MODULE check_parallelism.cpp)
target_link_libraries(check_parallelism PRIVATE Boost::json meld::core)

add_test(NAME job:check_parallelism_default
         COMMAND meld -c ${CMAKE_CURRENT_SOURCE_DIR}/check_parallelism_default.jsonnet)
add_test(NAME job:check_parallelism_cli
         COMMAND meld -c ${CMAKE_CURRENT_SOURCE_DIR}/check_parallelism_cli.jsonnet -j 7)
add_test(NAME job:check_parallelism_config
         COMMAND meld -c ${CMAKE_CURRENT_SOURCE_DIR}/check_parallelism_config.jsonnet)
add_test(NAME job:check_parallelism_cli_over_config
         COMMAND meld -c ${CMAKE_CURRENT_SOURCE_DIR}/check_parallelism_cli_over_config.jsonnet -j 9)
set_tests_properties(
  job:check_parallelism_default
  job:check_parallelism_cli
  job:check_parallelism_config
  job:check_parallelism_cli_over_config
  PROPERTIES ENVIRONMENT "MELD_PLUGIN_PATH=${CMAKE_CURRENT_BINARY_DIR}")
