#ifndef meld_concurrency_hpp
#define meld_concurrency_hpp

#include "oneapi/tbb/flow_graph.h"
#include "oneapi/tbb/global_control.h"

namespace meld::concurrency {
  inline constexpr auto unlimited = tbb::flow::unlimited;
  inline constexpr auto serial = tbb::flow::serial;

  class max_allowed_parallelism {
  public:
    explicit max_allowed_parallelism(std::size_t const n) :
      control_{tbb::global_control::max_allowed_parallelism, n}
    {
    }

  private:
    tbb::global_control control_;
  };
}

#endif /* meld_concurrency_hpp */
