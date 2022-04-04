#ifndef sand_core_source_worker_hpp
#define sand_core_source_worker_hpp

#include "sand/core/node.hpp"

#include <memory>

namespace sand {
  class source_worker {
  public:
    virtual ~source_worker() = default;

    // Replace with unique_ptr, once I figure out how to handle
    // std::function<void()> copyability issues.
    std::shared_ptr<node>
    next()
    {
      return data();
    }

  private:
    virtual std::shared_ptr<node> data() = 0;
  };

  using source_creator_t = std::unique_ptr<source_worker>(std::size_t);
}

#endif /* sand_core_source_worker_hpp */
