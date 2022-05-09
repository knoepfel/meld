# Interface library
add_library(meld_utilities_int INTERFACE)
target_include_directories(meld_utilities_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")

add_library(meld::utilities ALIAS meld_utilities_int)
