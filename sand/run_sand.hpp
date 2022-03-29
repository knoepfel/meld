#ifndef sand_run_sand_hh
#define sand_run_sand_hh

#include "sand/core/node.hpp"

#include <cstddef>
#include <memory>

namespace sand {

  std::unique_ptr<node> next();
  void run(std::size_t n);
  void process(std::size_t i, node& data);
}

#endif
