add_library(meld_metaprogramming SHARED function_name.cpp)
target_include_directories(meld_metaprogramming PRIVATE ${PROJECT_SOURCE_DIR} ${BACKTRACE_INC})
target_link_libraries(meld_metaprogramming
                      PRIVATE Boost::stacktrace_basic ${BACKTRACE_LIB}
                      PUBLIC Boost::headers)
target_compile_definitions(meld_metaprogramming
                           PRIVATE
                             BOOST_STACKTRACE_USE_BACKTRACE
                             BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED)



# Interface library
add_library(meld_metaprogramming_int INTERFACE)
target_include_directories(meld_metaprogramming_int INTERFACE
   "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>"
   "$<INSTALL_INTERFACE:include>")

target_link_libraries(meld_metaprogramming_int INTERFACE meld_metaprogramming)

add_library(meld::metaprogramming ALIAS meld_metaprogramming_int)
