#include "sand/run_sand.hpp"
#include "sand/core/data_processor.hpp"
#include "sand/core/source.hpp"
#include "sand/core/test_module.hpp"

#include <iostream>

namespace sand {
  void
  run_it(std::size_t const n)
  {
    std::cout << "Running sand\n";
    data_processor<source, test::module_to_use> processor{n};
    processor.run_to_completion();
  }
}
