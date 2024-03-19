#include "meld/core/consumer.hpp"

namespace meld {
  consumer::consumer(std::string name, std::vector<std::string> predicates) :
    name_{std::move(name)}, predicates_{std::move(predicates)}
  {
  }

  std::string const& consumer::name() const noexcept { return name_; }

  std::vector<std::string> const& consumer::when() const noexcept { return predicates_; }
}
