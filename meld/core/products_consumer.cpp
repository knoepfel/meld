#include "meld/core/products_consumer.hpp"

namespace meld {

  products_consumer::products_consumer(qualified_name name, std::vector<std::string> predicates) :
    consumer{std::move(name), std::move(predicates)}
  {
  }

  products_consumer::~products_consumer() = default;

  std::size_t products_consumer::num_inputs() const { return input().size(); }

  tbb::flow::receiver<message>& products_consumer::port(specified_label const& product_label)
  {
    return port_for(product_label);
  }

}
