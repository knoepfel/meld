#include "meld/core/declared_filter.hpp"

namespace meld {
  declared_filter::declared_filter(std::string name, std::vector<std::string> preceding_filters) :
    consumer{move(preceding_filters)}, name_{move(name)}
  {
  }

  declared_filter::~declared_filter() = default;
  std::string const& declared_filter::name() const noexcept { return name_; }
}
