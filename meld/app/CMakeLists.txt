configure_file(version.cpp.in version.cpp @ONLY)

add_library(run_meld SHARED run_meld.cpp version.cpp)
target_link_libraries(run_meld PRIVATE Boost::boost meld::core)

if (NOT JSONNET_LIB)
  message(FATAL_ERROR "JSONNET_LIB has not been specified.")
endif()

add_executable(meld meld.cpp)
target_link_directories(meld PRIVATE ${JSONNET_LIB})
target_link_libraries(meld PRIVATE Boost::boost Boost::program_options run_meld meld::core jsonnet++)
