#include "sand/run_sand.hpp"
#include "sand/core/data_processor.hpp"
#include "sand/core/load_module.hpp"
#include "sand/core/source.hpp"

#include <iostream>

namespace sand {
  void
  run_it(std::size_t const n, std::string const& source, std::string const& module)
  {
    std::cout << "Running sand\n";
    data_processor processor{n, load_source(source, n), load_module(module)};
    processor.run_to_completion();
  }
}
