add_library(meld_model SHARED
  level_counter.cpp
  level_hierarchy.cpp
  level_id.cpp
  product_matcher.cpp
  product_store.cpp
  products.cpp
  qualified_name.cpp
)
target_include_directories(meld_model PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(meld_model
  PUBLIC Boost::boost
  PRIVATE meld::graph spdlog::spdlog meld::utilities)

# Interface library
add_library(meld_model_int INTERFACE)
target_include_directories(meld_model_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_model_int INTERFACE meld_model spdlog::spdlog TBB::tbb)

add_library(meld::model ALIAS meld_model_int)

install(TARGETS meld_model)
