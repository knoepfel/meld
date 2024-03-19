#include "meld/core/declared_reduction.hpp"

namespace meld {
  declared_reduction::declared_reduction(std::string name, std::vector<std::string> predicates) :
    products_consumer{std::move(name), std::move(predicates)}
  {
  }

  declared_reduction::~declared_reduction() = default;
}
