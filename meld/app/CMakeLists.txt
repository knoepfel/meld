find_package(CLI11 REQUIRED)

configure_file(version.cpp.in version.cpp @ONLY)

# add_library(run_meld SHARED run_meld.cpp version.cpp)
# target_link_libraries(run_meld PRIVATE Boost::boost meld::core)
#
# add_executable(meld meld.cpp)
# target_link_libraries(meld PRIVATE Boost::boost CLI11::CLI11 run_meld meld::core)
# target_link_options(meld PRIVATE -Wl,-export_dynamic)
