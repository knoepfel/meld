find_package(Catch2 REQUIRED)

function(create_component COMPONENT_NAME)
  add_library(${COMPONENT_NAME} MODULE ${COMPONENT_NAME}.cpp)
  target_link_libraries(${COMPONENT_NAME} PRIVATE Boost::boost meld::core test_levels)
  set_target_properties(${COMPONENT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
endfunction()

function(add_unit_test BASENAME)
  set(multiValueArgs LIBRARIES)
  cmake_parse_arguments(UT "" "" "${multiValueArgs}" ${ARGN})
  add_executable(${BASENAME} ${BASENAME}.cpp)
  target_link_libraries(${BASENAME} PRIVATE Catch2::Catch2 ${UT_LIBRARIES})
  target_compile_definitions(${BASENAME} PRIVATE CATCH_CONFIG_MAIN)
  add_test(NAME ${BASENAME} COMMAND ${BASENAME})
endfunction()

add_library(test_levels SHARED data_levels.cpp)
target_link_libraries(test_levels PRIVATE meld::core TBB::tbb)

create_component(test_module_multiple_levels)
create_component(test_module_run)
create_component(test_source)

add_test(NAME meld_t COMMAND meld --config ${CMAKE_CURRENT_SOURCE_DIR}/test_meld.json)
add_test(NAME meld_with_deps_t COMMAND meld --config ${CMAKE_CURRENT_SOURCE_DIR}/test_meld_with_deps.json)

add_unit_test(meld_check_t LIBRARIES meld::core Boost::boost test_levels)
add_unit_test(transition_t LIBRARIES meld::core)
add_unit_test(gatekeeper_t LIBRARIES TBB::tbb meld::core test_levels)
add_unit_test(concurrency_tags_t LIBRARIES meld::core)
