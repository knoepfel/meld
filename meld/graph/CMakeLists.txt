add_library(meld_graph SHARED
  gatekeeper_node.cpp
  module_worker.cpp
  data_node.cpp
  serializer_node.cpp
  transition.cpp
  transition_graph.cpp
  )
target_include_directories(meld_graph PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(meld_graph PRIVATE TBB::tbb Boost::boost)

# Interface library
add_library(meld_graph_int INTERFACE)
target_include_directories(meld_graph_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_graph_int INTERFACE meld_graph TBB::tbb)

add_library(meld::graph ALIAS meld_graph_int)
