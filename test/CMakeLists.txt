function(create_component COMPONENT_NAME)
  add_library(${COMPONENT_NAME} MODULE ${COMPONENT_NAME}.cpp)
  target_include_directories(${COMPONENT_NAME} PRIVATE ${PROJECT_SOURCE_DIR})
  target_link_libraries(${COMPONENT_NAME} PRIVATE Boost::boost sand_core sand_levels)
  set_target_properties(${COMPONENT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
endfunction()

create_component(test_module_multiple_levels)
create_component(test_module_run)
create_component(test_source)

add_test(NAME sand_t COMMAND sand --config ${CMAKE_CURRENT_SOURCE_DIR}/test_sand.json)
