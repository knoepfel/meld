add_library(meld_graph SHARED
  serializer_node.cpp
)
target_include_directories(meld_graph PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(meld_graph PRIVATE Boost::boost TBB::tbb meld::utilities fmt::fmt)

# Interface library
add_library(meld_graph_int INTERFACE)
target_include_directories(meld_graph_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_graph_int INTERFACE meld_graph TBB::tbb)

add_library(meld::graph ALIAS meld_graph_int)
