#include "meld/core/declared_reduction.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"

namespace meld {

  declared_reduction::declared_reduction(std::string name, std::size_t concurrency) :
    name_{move(name)}, concurrency_{concurrency}
  {
  }

  declared_reduction::~declared_reduction() = default;

  std::string const&
  declared_reduction::name() const noexcept
  {
    return name_;
  }

  std::size_t
  declared_reduction::concurrency() const noexcept
  {
    return concurrency_;
  }

  tbb::flow::receiver<product_store_ptr>&
  declared_reduction::port(std::string const& product_name)
  {
    return port_for(product_name);
  }
}
