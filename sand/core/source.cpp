#include "sand/core/source.hpp"
#include "sand/core/data_levels.hpp"

namespace sand {
  source::source(std::size_t const n) : num_nodes_{n} {}

  std::shared_ptr<node>
  source::next()
  {
    if (cursor_ < num_nodes_) {
      auto id = cursor_;
      ++cursor_;
      return std::make_shared<run>(cursor_);
    }
    return nullptr;
  }
}
