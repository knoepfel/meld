find_package(Catch2 REQUIRED)

add_library(test_levels SHARED data_levels.cpp)
target_link_libraries(test_levels PRIVATE sand::core)

function(create_component COMPONENT_NAME)
  add_library(${COMPONENT_NAME} MODULE ${COMPONENT_NAME}.cpp)
  target_link_libraries(${COMPONENT_NAME} PRIVATE Boost::boost sand::core test_levels)
  set_target_properties(${COMPONENT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
endfunction()

create_component(test_module_multiple_levels)
create_component(test_module_run)
create_component(test_source)

add_test(NAME sand_t COMMAND sand --config ${CMAKE_CURRENT_SOURCE_DIR}/test_sand.json)
add_test(NAME sand_with_deps_t COMMAND sand --config ${CMAKE_CURRENT_SOURCE_DIR}/test_sand_with_deps.json)

add_executable(sand_check sand_check_t.cpp)
target_link_libraries(sand_check PRIVATE sand::core Catch2::Catch2 Boost::boost TBB::tbb test_levels)
target_compile_definitions(sand_check PRIVATE CATCH_CONFIG_MAIN)
add_test(NAME sand_check_t COMMAND sand_check)

add_executable(transition transition_t.cpp)
target_link_libraries(transition PRIVATE sand::core Catch2::Catch2)
target_compile_definitions(transition PRIVATE CATCH_CONFIG_MAIN)
add_test(NAME transition_t COMMAND transition)
