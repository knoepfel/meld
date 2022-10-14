#include "meld/core/declared_transform.hpp"

namespace meld {
  declared_transform::declared_transform(std::string name,
                                         std::vector<std::string> preceding_filters) :
    consumer{move(preceding_filters)}, name_{move(name)}
  {
  }

  declared_transform::~declared_transform() = default;
  std::string const& declared_transform::name() const noexcept { return name_; }
}
