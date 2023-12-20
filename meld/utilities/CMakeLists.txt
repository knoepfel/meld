add_library(meld_utilities SHARED hashing.cpp resource_usage.cpp)
target_include_directories(meld_utilities PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(meld_utilities PRIVATE Boost::boost spdlog::spdlog)

# Interface library
add_library(meld_utilities_int INTERFACE)
target_include_directories(meld_utilities_int INTERFACE
  "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
  "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_utilities_int INTERFACE meld_utilities spdlog::spdlog)

add_library(meld::utilities ALIAS meld_utilities_int)
