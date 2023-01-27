find_package(Catch2 REQUIRED)

find_program(DOT dot)

function(add_unit_test BASENAME)
  set(multiValueArgs LIBRARIES)
  cmake_parse_arguments(UT "" "" "${multiValueArgs}" ${ARGN})
  add_executable(${BASENAME} ${BASENAME}.cpp)
  target_link_libraries(${BASENAME} PRIVATE ${UT_LIBRARIES})
  set_target_properties(${BASENAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${BASENAME}.d)
  add_test(NAME ${BASENAME} COMMAND ${BASENAME} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${BASENAME}.d)
  set_tests_properties(${BASENAME} PROPERTIES ENVIRONMENT SPDLOG_LEVEL=debug)
endfunction()

function(add_catch_test BASENAME)
  set(optionalArgs TEST_DOT_GRAPH)
  set(multiValueArgs LIBRARIES)
  cmake_parse_arguments(UT "${optionalArgs}" "" "${multiValueArgs}" ${ARGN})
  add_unit_test(${BASENAME} LIBRARIES Catch2::Catch2 ${UT_LIBRARIES})
  target_compile_definitions(${BASENAME} PRIVATE CATCH_CONFIG_MAIN)
  if(UT_TEST_DOT_GRAPH)
    if(NOT DOT)
      message(WARNING "dot program not found; all TEST_DOT_GRAPH options will be ignored.")
    else()
      set(ORIG_TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/${BASENAME}.d)
      set(TEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/dot_${BASENAME}.d)
      file(MAKE_DIRECTORY ${TEST_DIR})
      add_test(NAME dot:${BASENAME}
        COMMAND bash -c "for g in \$(ls ../${BASENAME}.d/*.gv); do dot -Tpdf ../${BASENAME}.d/\${g} -o \$(basename \${g%.gv}).pdf; done"
        WORKING_DIRECTORY ${TEST_DIR})
      set_tests_properties(dot:${BASENAME} PROPERTIES DEPENDS ${BASENAME})
    endif()
  endif()
endfunction()

# include(FetchContent)
#
# # You can configure this for your need, presumbably you want speificy a git tag here instead of a branch
# FetchContent_Declare(sqliteOrm
#   GIT_REPOSITORY https://github.com/fnc12/sqlite_orm
#   GIT_TAG        origin/dev
# )
# FetchContent_MakeAvailable(sqliteOrm)

add_unit_test(concepts LIBRARIES meld::core)
add_unit_test(iterator)
add_unit_test(string_literal LIBRARIES meld::utilities)
add_unit_test(type_deduction LIBRARIES meld::metaprogramming)

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

add_catch_test(cached_execution LIBRARIES meld::core Boost::json TEST_DOT_GRAPH)
add_catch_test(cached_product_stores LIBRARIES meld::core)
add_catch_test(class_registration LIBRARIES meld::core Boost::json)
add_catch_test(dynamic_join LIBRARIES meld::graph meld::utilities)
add_catch_test(filter_impl LIBRARIES meld::core)
add_catch_test(filter LIBRARIES meld::core Boost::json TEST_DOT_GRAPH)
add_catch_test(function_registration LIBRARIES meld::core Boost::json)
add_catch_test(function_name LIBRARIES meld::metaprogramming)
add_catch_test(hierarchical_nodes LIBRARIES Boost::json TBB::tbb meld::core TEST_DOT_GRAPH)
add_catch_test(make_edges LIBRARIES meld::graph)
add_catch_test(multiple_function_registration LIBRARIES Boost::json meld::core)
add_catch_test(level_counting LIBRARIES meld::model)
add_catch_test(product_handle LIBRARIES meld::core)
add_catch_test(product_store LIBRARIES meld::core)
add_catch_test(replicated LIBRARIES TBB::tbb meld::utilities spdlog::spdlog)
add_catch_test(serializer LIBRARIES meld::core TBB::tbb)
add_catch_test(splitter LIBRARIES Boost::json meld::core TBB::tbb TEST_DOT_GRAPH)
add_catch_test(transition LIBRARIES meld::model)

add_subdirectory(benchmarks)
add_subdirectory(max-parallelism)
add_subdirectory(memory-checks)
add_subdirectory(plugins)
add_subdirectory(utilities)
