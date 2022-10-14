#ifndef meld_core_consumer_hpp
#define meld_core_consumer_hpp

#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <span>
#include <string>
#include <vector>

namespace meld {
  class consumer {
  public:
    explicit consumer(std::vector<std::string> preceding_filters);

    virtual ~consumer();

    std::vector<std::string> const& filtered_by() const noexcept;
    std::size_t num_inputs() const;

    tbb::flow::receiver<message>& port(std::string const& product_name);
    virtual std::span<std::string const, std::dynamic_extent> input() const = 0;

  private:
    virtual tbb::flow::receiver<message>& port_for(std::string const& product_name) = 0;

    std::vector<std::string> preceding_filters_;
  };
}

#endif /* meld_core_consumer_hpp */
