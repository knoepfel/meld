add_library(meld_metaprogramming SHARED function_name.cpp)
target_include_directories(meld_metaprogramming PRIVATE ${PROJECT_SOURCE_DIR})
target_link_libraries(meld_metaprogramming PRIVATE Boost::stacktrace_basic)
target_compile_definitions(meld_metaprogramming PUBLIC _GNU_SOURCE)

# Interface library
add_library(meld_metaprogramming_int INTERFACE)
target_include_directories(meld_metaprogramming_int INTERFACE
   "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
   "$<INSTALL_INTERFACE:include>")
target_link_libraries(meld_metaprogramming_int INTERFACE meld_metaprogramming)

add_library(meld::metaprogramming ALIAS meld_metaprogramming_int)
