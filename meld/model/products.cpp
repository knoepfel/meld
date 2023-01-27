#include "meld/model/products.hpp"

#include <string>

namespace meld {
  bool products::contains(std::string const& product_name) const
  {
    return products_.contains(product_name);
  }

  products::const_iterator products::begin() const noexcept { return products_.begin(); }
  products::const_iterator products::end() const noexcept { return products_.end(); }
}
