#include "meld/core/declared_transform.hpp"

namespace meld {
  declared_transform::declared_transform(std::string name,
                                         std::vector<std::string> preceding_filters,
                                         std::vector<std::string> release_stores) :
    products_consumer{move(name), move(preceding_filters), move(release_stores)}
  {
  }

  declared_transform::~declared_transform() = default;
}
