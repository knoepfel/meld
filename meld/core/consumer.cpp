#include "meld/core/consumer.hpp"

namespace meld {

  consumer::consumer(std::vector<std::string> preceding_filters) :
    preceding_filters_{move(preceding_filters)}
  {
  }

  consumer::~consumer() = default;

  std::vector<std::string> const& consumer::filtered_by() const noexcept
  {
    return preceding_filters_;
  }
  std::size_t consumer::num_inputs() const { return input().size(); }

  tbb::flow::receiver<message>& consumer::port(std::string const& product_name)
  {
    return port_for(product_name);
  }
}
