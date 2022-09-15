#include "meld/core/declared_splitter.hpp"

namespace meld {

  generator::generator(product_store_ptr const& parent) : parent_{parent} {}

  void
  generator::make_child(std::size_t const i)
  {
    auto child = parent_->make_child(i);
    debug(" Made child with ID: ", child->id());
  }

  declared_splitter::declared_splitter(std::string name, std::size_t concurrency) :
    name_{move(name)}, concurrency_{concurrency}
  {
  }

  declared_splitter::~declared_splitter() = default;

  std::string const&
  declared_splitter::name() const noexcept
  {
    return name_;
  }

  std::size_t
  declared_splitter::concurrency() const noexcept
  {
    return concurrency_;
  }

  tbb::flow::receiver<message>&
  declared_splitter::port(std::string const& product_name)
  {
    return port_for(product_name);
  }
}
