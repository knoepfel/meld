#ifndef sand_core_source_worker_hpp
#define sand_core_source_worker_hpp

#include "sand/core/node.hpp"

#include "boost/json.hpp"

#include <memory>
#include <vector>

namespace sand {
  class source_worker {
  public:
    virtual ~source_worker() = default;

    // Replace with unique_ptr, once I figure out how to handle
    // std::function<void()> copyability issues.
    transition_packages
    next()
    {
      return next_transitions();
    }

  private:
    virtual transition_packages next_transitions() = 0;
  };

  using source_creator_t = std::unique_ptr<source_worker>(boost::json::value const&);
}

#endif /* sand_core_source_worker_hpp */
