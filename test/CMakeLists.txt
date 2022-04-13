add_library(test_module_multiple_levels MODULE test_module_multiple_levels.cpp)
target_include_directories(test_module_multiple_levels PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(test_module_multiple_levels PRIVATE Boost::boost sand_core)
set_target_properties(test_module_multiple_levels PROPERTIES CXX_VISIBILITY_PRESET hidden)

add_library(test_module_run MODULE test_module_run.cpp)
target_include_directories(test_module_run PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(test_module_run PRIVATE Boost::boost sand_core)
set_target_properties(test_module_run PROPERTIES CXX_VISIBILITY_PRESET hidden)

add_library(test_source MODULE test_source.cpp)
target_include_directories(test_source PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(test_source PRIVATE Boost::boost sand_core)
set_target_properties(test_source PROPERTIES CXX_VISIBILITY_PRESET hidden)

add_test(NAME sand_t COMMAND sand --config ${CMAKE_CURRENT_SOURCE_DIR}/test_sand.json)
