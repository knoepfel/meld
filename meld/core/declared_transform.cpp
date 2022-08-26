#include "meld/core/declared_transform.hpp"
#include "meld/core/product_store.hpp"

namespace meld {

  declared_transform::declared_transform(std::string name, std::size_t concurrency) :
    name_{move(name)}, concurrency_{concurrency}
  {
  }

  declared_transform::~declared_transform() = default;

  // void
  // declared_transform::invoke(std::span<product_store_ptr> stores) const
  // {
  //   if (std::any_of(cbegin(stores), cend(stores), [](auto const& store) {
  //         return store->has(action::flush);
  //       })) {
  //     return;
  //   }
  //   invoke_(stores);
  // }

  std::string const&
  declared_transform::name() const noexcept
  {
    return name_;
  }

  std::size_t
  declared_transform::concurrency() const noexcept
  {
    return concurrency_;
  }

  tbb::flow::receiver<product_store_ptr>&
  declared_transform::port(std::string const& product_name)
  {
    return port_for(product_name);
  }
}
