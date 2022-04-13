add_library(sand_core SHARED load_module.cpp node.cpp)
target_include_directories(sand_core PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(sand_core PRIVATE Boost::boost)

add_library(sand_levels SHARED data_levels.cpp)
target_include_directories(sand_levels PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(sand_levels PRIVATE sand_core)

# We'll use C++17's filesystem insteead of Boost's
target_compile_definitions(sand_core PRIVATE BOOST_DLL_USE_STD_FS)
