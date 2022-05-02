find_package(Catch2 REQUIRED)

add_library(test_levels SHARED data_levels.cpp)
target_link_libraries(test_levels PRIVATE meld::core TBB::tbb)

function(create_component COMPONENT_NAME)
  add_library(${COMPONENT_NAME} MODULE ${COMPONENT_NAME}.cpp)
  target_link_libraries(${COMPONENT_NAME} PRIVATE Boost::boost meld::core test_levels)
  set_target_properties(${COMPONENT_NAME} PROPERTIES CXX_VISIBILITY_PRESET hidden)
endfunction()

create_component(test_module_multiple_levels)
create_component(test_module_run)
create_component(test_source)

add_test(NAME meld_t COMMAND meld --config ${CMAKE_CURRENT_SOURCE_DIR}/test_meld.json)
add_test(NAME meld_with_deps_t COMMAND meld --config ${CMAKE_CURRENT_SOURCE_DIR}/test_meld_with_deps.json)

add_executable(meld_check meld_check_t.cpp)
target_link_libraries(meld_check PRIVATE meld::core Catch2::Catch2 Boost::boost TBB::tbb test_levels)
target_compile_definitions(meld_check PRIVATE CATCH_CONFIG_MAIN)
add_test(NAME meld_check_t COMMAND meld_check)

add_executable(transition_t transition_t.cpp)
target_link_libraries(transition_t PRIVATE meld::core Catch2::Catch2)
target_compile_definitions(transition_t PRIVATE CATCH_CONFIG_MAIN)
add_test(NAME transition_t COMMAND transition_t)

add_executable(gatekeeper_t gatekeeper_t.cpp)
target_link_libraries(gatekeeper_t PRIVATE Catch2::Catch2 TBB::tbb meld::core test_levels)
target_compile_definitions(gatekeeper_t PRIVATE CATCH_CONFIG_MAIN)
add_test(NAME gatekeeper_t COMMAND gatekeeper_t)
