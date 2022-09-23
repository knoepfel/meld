#include "meld/core/declared_transform.hpp"

namespace meld {

  declared_transform::declared_transform(std::string name, std::size_t concurrency) :
    name_{move(name)}, concurrency_{concurrency}
  {
  }

  declared_transform::~declared_transform() = default;

  std::string const& declared_transform::name() const noexcept { return name_; }

  std::size_t declared_transform::concurrency() const noexcept { return concurrency_; }

  tbb::flow::receiver<message>& declared_transform::port(std::string const& product_name)
  {
    return port_for(product_name);
  }
}
