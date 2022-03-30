#ifndef sand_core_source_hh
#define sand_core_source_hh

#include "sand/core/node.hpp"

#include <memory>

namespace sand {
  class source {
  public:
    explicit source(std::size_t n);
    std::shared_ptr<node> next(); // Replace with unique_ptr, once I figure out how to
                                  // handle std::function<void()> copyability issues.

  private:
    std::size_t num_nodes_;
    std::size_t cursor_{0};
  };
}

#endif
