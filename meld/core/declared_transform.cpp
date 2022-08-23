#include "meld/core/declared_transform.hpp"
#include "meld/core/product_store.hpp"

namespace meld {

  declared_transform::declared_transform(std::string name,
                                         std::size_t concurrency,
                                         std::vector<std::string> input_keys,
                                         std::vector<std::string> output_keys) :
    name_{move(name)},
    concurrency_{concurrency},
    input_keys_{move(input_keys)},
    output_keys_{move(output_keys)}
  {
  }

  declared_transform::~declared_transform() = default;

  void
  declared_transform::invoke(product_store& store) const
  {
    if (store.is_flush()) {
      return;
    }
    invoke_(store);
  }

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

  std::vector<std::string> const&
  declared_transform::input() const noexcept
  {
    return input_keys_;
  }
  std::vector<std::string> const&
  declared_transform::output() const noexcept
  {
    return output_keys_;
  }

}
