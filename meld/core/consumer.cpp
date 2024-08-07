#include "meld/core/consumer.hpp"

namespace meld {
  consumer::consumer(qualified_name name, std::vector<std::string> predicates) :
    name_{std::move(name)}, predicates_{std::move(predicates)}
  {
  }

  std::string consumer::full_name() const { return name_.full(); }

  std::string const& consumer::module() const noexcept { return name_.module(); }
  std::string const& consumer::name() const noexcept { return name_.name(); }

  std::vector<std::string> const& consumer::when() const noexcept { return predicates_; }
}
