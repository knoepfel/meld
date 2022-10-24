# Interface library
add_library(meld_metaprogramming_int INTERFACE)
target_include_directories(meld_metaprogramming_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")

add_library(meld::metaprogramming ALIAS meld_metaprogramming_int)
