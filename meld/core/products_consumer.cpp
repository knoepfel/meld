#include "meld/core/products_consumer.hpp"

namespace meld {

  products_consumer::products_consumer(std::string name,
                                       std::vector<std::string> preceding_filters,
                                       std::vector<std::string> receive_stores) :
    consumer{move(name), move(preceding_filters), move(receive_stores)}
  {
  }

  products_consumer::~products_consumer() = default;

  std::size_t products_consumer::num_inputs() const { return input().size(); }

  tbb::flow::receiver<message>& products_consumer::port(std::string const& product_name)
  {
    return port_for(product_name);
  }

}