add_library(meld_model SHARED
  level_hierarchy.cpp
  level_order.cpp
  product_store.cpp
  product_store_factory.cpp
  transition.cpp
)
target_include_directories(meld_model PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(meld_model
  PRIVATE meld::graph spdlog::spdlog meld::utilities)

# Interface library
add_library(meld_model_int INTERFACE)
target_include_directories(meld_model_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_model_int INTERFACE meld_model spdlog::spdlog TBB::tbb)

add_library(meld::model ALIAS meld_model_int)
