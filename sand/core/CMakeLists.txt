add_library(sand_core SHARED source.cpp load_module.cpp)
target_include_directories(sand_core PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(sand_core PRIVATE Boost::boost)
