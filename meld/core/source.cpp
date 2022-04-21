#include "meld/core/source.hpp"
#include "meld/core/data_levels.hpp"

#include <iostream>

namespace meld {
  source::source(std::size_t const n) : num_nodes_{n} {}

  std::shared_ptr<node>
  source::next()
  {
    if (cursor_ < num_nodes_) {
      ++cursor_;
      std::cout << "Creating run " << cursor_ << '\n';
      return std::make_shared<run>(cursor_);
    }
    return nullptr;
  }
}
