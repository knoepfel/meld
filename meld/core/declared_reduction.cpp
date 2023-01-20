#include "meld/core/declared_reduction.hpp"

namespace meld {
  declared_reduction::declared_reduction(std::string name,
                                         std::vector<std::string> preceding_filters,
                                         std::vector<std::string> receive_stores) :
    products_consumer{move(name), move(preceding_filters), move(receive_stores)}
  {
  }

  declared_reduction::~declared_reduction() = default;
}
