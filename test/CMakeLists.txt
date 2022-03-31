add_test(NAME sand_t COMMAND sand -n 10)

add_library(test_module SHARED test_module.cpp)
target_include_directories(test_module PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(test_module PRIVATE Boost::boost)
