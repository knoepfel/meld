#include "meld/core/declared_reduction.hpp"

namespace meld {
  declared_reduction::declared_reduction(std::string name,
                                         std::vector<std::string> preceding_filters) :
    consumer{move(preceding_filters)}, name_{move(name)}
  {
  }

  declared_reduction::~declared_reduction() = default;
  std::string const& declared_reduction::name() const noexcept { return name_; }
}
