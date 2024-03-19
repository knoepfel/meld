#include "meld/core/declared_monitor.hpp"

namespace meld {
  declared_monitor::declared_monitor(std::string name, std::vector<std::string> predicates) :
    products_consumer{std::move(name), std::move(predicates)}
  {
  }

  declared_monitor::~declared_monitor() = default;
}
