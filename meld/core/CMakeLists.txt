add_library(meld_core SHARED
  data_processor.cpp
  gatekeeper_node.cpp
  load_module.cpp
  module_manager.cpp
  module_worker.cpp
  node.cpp
  transition.cpp
  transition_graph.cpp
  )
target_include_directories(meld_core PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(meld_core PRIVATE Boost::boost TBB::tbb)

# We'll use C++17's filesystem insteead of Boost's
target_compile_definitions(meld_core PRIVATE BOOST_DLL_USE_STD_FS)

# Interface library
add_library(meld_core_int INTERFACE)
target_include_directories(meld_core_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_core_int INTERFACE meld_core TBB::tbb)

add_library(meld::core ALIAS meld_core_int)
