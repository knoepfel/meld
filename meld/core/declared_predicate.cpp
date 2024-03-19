#include "meld/core/declared_predicate.hpp"

namespace meld {
  declared_predicate::declared_predicate(std::string name, std::vector<std::string> predicates) :
    products_consumer{std::move(name), std::move(predicates)}
  {
  }

  declared_predicate::~declared_predicate() = default;
}
