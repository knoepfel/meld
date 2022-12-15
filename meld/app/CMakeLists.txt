configure_file(version.cpp.in version.cpp @ONLY)

add_library(run_meld SHARED
  load_module.cpp
  run.cpp
  version.cpp)
target_link_libraries(run_meld PRIVATE Boost::boost Boost::json meld::core)

# We'll use C++17's filesystem insteead of Boost's
target_compile_definitions(run_meld PRIVATE BOOST_DLL_USE_STD_FS)

if (NOT JSONNET_LIB)
  message(FATAL_ERROR "JSONNET_LIB has not been specified.")
endif()

add_executable(meld meld.cpp)
target_link_directories(meld PRIVATE ${JSONNET_LIB})
target_link_libraries(meld PRIVATE Boost::json Boost::program_options run_meld meld::core jsonnet++)
