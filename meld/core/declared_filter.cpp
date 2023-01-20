#include "meld/core/declared_filter.hpp"

namespace meld {
  declared_filter::declared_filter(std::string name,
                                   std::vector<std::string> preceding_filters,
                                   std::vector<std::string> receive_stores) :
    products_consumer{move(name), move(preceding_filters), move(receive_stores)}
  {
  }

  declared_filter::~declared_filter() = default;
}
