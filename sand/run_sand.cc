#include "sand/run_sand.hh"

#include <iostream>

namespace sand {
  std::unique_ptr<node>
  next()
  {
    return std::make_unique<node>();
  }

  void
  process(std::size_t const i, std::size_t const n, node& data)
  {
    std::cout << "Processing data "
              << "(" << i + 1 << '/' << n << " nodes)\n";
  }

  void
  run(std::size_t const n)
  {
    std::cout << "Running sand\n";
    for (std::size_t i = 0; i != n; ++i) {
      auto data = next();
      process(i, n, *data);
    }
  }
}
