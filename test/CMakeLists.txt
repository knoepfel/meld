add_test(NAME sand_t COMMAND sand -n 10)

add_library(test_module SHARED test_module.cpp)
target_include_directories(test_module PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(test_module PRIVATE Boost::boost)
target_link_options(test_module PRIVATE -fvisibility=hidden)

add_library(test_source SHARED test_source.cpp)
target_include_directories(test_source PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(test_source PRIVATE Boost::boost)
target_link_options(test_source PRIVATE -fvisibility=hidden)
