configure_file(version.cpp.in version.cpp @ONLY)

add_library(run_meld SHARED
  load_module.cpp
  run.cpp
  version.cpp)
target_link_libraries(run_meld PRIVATE Boost::json meld::core)

# We'll use C++17's filesystem instead of Boost's
target_compile_definitions(run_meld PRIVATE BOOST_DLL_USE_STD_FS)

add_executable(meld meld.cpp)
target_link_libraries(meld PRIVATE Boost::json Boost::program_options run_meld meld::core jsonnet::lib)
