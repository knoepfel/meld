#include "meld/core/declared_reduction.hpp"

namespace meld {
  declared_reduction::declared_reduction(std::string name,
                                         std::vector<std::string> preceding_filters) :
    products_consumer{std::move(name), std::move(preceding_filters)}
  {
  }

  declared_reduction::~declared_reduction() = default;
}
