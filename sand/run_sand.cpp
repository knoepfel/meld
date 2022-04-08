#include "sand/run_sand.hpp"
#include "sand/core/data_processor.hpp"
#include "sand/core/load_module.hpp"

#include <iostream>

namespace sand {
  void
  run_it(configurations_t const& configurations)
  {
    std::cout << "Running sand\n";
    data_processor processor{load_source(configurations.at("source")),
                             load_module(configurations.at("module"))};
    processor.run_to_completion();
  }
}
