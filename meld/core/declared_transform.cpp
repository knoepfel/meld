#include "meld/core/declared_transform.hpp"

namespace meld {
  declared_transform::declared_transform(std::string name,
                                         std::vector<std::string> preceding_filters) :
    products_consumer{move(name), move(preceding_filters)}
  {
  }

  declared_transform::~declared_transform() = default;
}
