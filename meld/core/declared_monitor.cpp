#include "meld/core/declared_monitor.hpp"

namespace meld {
  declared_monitor::declared_monitor(std::string name, std::vector<std::string> preceding_filters) :
    consumer{move(preceding_filters)}, name_{move(name)}
  {
  }

  declared_monitor::~declared_monitor() = default;
  std::string const& declared_monitor::name() const noexcept { return name_; }
}
