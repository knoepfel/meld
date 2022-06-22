add_library(meld_utilities SHARED demangle_symbol.cpp)
target_include_directories(meld_utilities PRIVATE ${PROJECT_SOURCE_DIR})

# Interface library
add_library(meld_utilities_int INTERFACE)
target_include_directories(meld_utilities_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_utilities_int INTERFACE meld_utilities)

add_library(meld::utilities ALIAS meld_utilities_int)
