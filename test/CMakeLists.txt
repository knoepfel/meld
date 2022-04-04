add_test(NAME sand_t COMMAND sand -n 10)

add_library(test_module MODULE test_module.cpp)
target_include_directories(test_module PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(test_module PRIVATE Boost::boost)
set_target_properties(test_module PROPERTIES CXX_VISIBILITY_PRESET hidden)

add_library(test_source MODULE test_source.cpp)
target_include_directories(test_source PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(test_source PRIVATE Boost::boost)
set_target_properties(test_source PROPERTIES CXX_VISIBILITY_PRESET hidden)
