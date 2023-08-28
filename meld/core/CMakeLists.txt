add_library(meld_core SHARED
  concurrency.cpp
  consumer.cpp
  declared_filter.cpp
  declared_monitor.cpp
  declared_output.cpp
  declared_reduction.cpp
  declared_splitter.cpp
  declared_transform.cpp
  edge_maker.cpp
  end_of_message.cpp
  filter/filter_impl.cpp
  filter/result_collector.cpp
  framework_graph.cpp
  message.cpp
  message_sender.cpp
  multiplexer.cpp
  products_consumer.cpp
  store_counters.cpp
  )
target_include_directories(meld_core PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(meld_core
  PRIVATE
    TBB::tbb
    meld::graph
    meld::metaprogramming
    meld::model
    meld::utilities
    spdlog::spdlog
)

# Interface library
add_library(meld_core_int INTERFACE)
target_include_directories(meld_core_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_core_int
  INTERFACE
  meld_core
  meld::metaprogramming
  meld::graph
  meld::model
  meld::utilities
  Boost::json
  TBB::tbb)

add_library(meld::core ALIAS meld_core_int)
