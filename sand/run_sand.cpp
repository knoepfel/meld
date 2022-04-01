#include "sand/run_sand.hpp"
#include "sand/core/data_processor.hpp"
#include "sand/core/source.hpp"

#include <iostream>

namespace sand {
  void
  run_it(std::size_t const n)
  {
    std::cout << "Running sand\n";
    data_processor processor{n};
    processor.run_to_completion();
  }
}
