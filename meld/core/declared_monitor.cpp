#include "meld/core/declared_monitor.hpp"

namespace meld {
  declared_monitor::declared_monitor(std::string name,
                                     std::vector<std::string> preceding_filters,
                                     std::vector<std::string> receive_stores) :
    products_consumer{std::move(name), std::move(preceding_filters), std::move(receive_stores)}
  {
  }

  declared_monitor::~declared_monitor() = default;
}
