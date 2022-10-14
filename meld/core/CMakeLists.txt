add_library(meld_core SHARED
  consumer.cpp
  declared_filter.cpp
  declared_output.cpp
  declared_splitter.cpp
  declared_transform.cpp
  declared_reduction.cpp
  edge_maker.cpp
  filter/filter_impl.cpp
  filter/result_collector.cpp
  framework_graph.cpp
  message.cpp
  multiplexer.cpp
  product_store.cpp
  )
target_include_directories(meld_core PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(meld_core PRIVATE Boost::boost TBB::tbb meld::graph meld::utilities)

# Interface library
add_library(meld_core_int INTERFACE)
target_include_directories(meld_core_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_core_int INTERFACE meld_core meld::graph meld::utilities TBB::tbb)

add_library(meld::core ALIAS meld_core_int)
