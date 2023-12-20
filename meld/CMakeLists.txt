add_subdirectory(app)
add_subdirectory(core)
add_subdirectory(graph)
add_subdirectory(metaprogramming)
add_subdirectory(model)
add_subdirectory(utilities)

# Interface library
add_library(meld_int INTERFACE)
target_include_directories(meld_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_int INTERFACE Boost::boost Boost::json meld::core)

add_library(meld::module ALIAS meld_int)
