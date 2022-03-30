#ifndef sand_core_data_levels_hpp
#define sand_core_data_levels_hpp

// Everything inside of here is considered "experiment-defined".

#include "sand/core/node.hpp"

#include <cstddef>

namespace sand {
  class run : public node {
  public:
    explicit run(std::size_t const id) : node{id} {}
  };
}

#endif
