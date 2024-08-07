#include "meld/model/qualified_name.hpp"

namespace meld {
  qualified_name::qualified_name() = default;

  qualified_name::qualified_name(char const* name) : qualified_name{"", name} {}
  qualified_name::qualified_name(std::string name) : qualified_name{"", name} {}

  qualified_name::qualified_name(std::string module, std::string name) :
    module_{std::move(module)}, name_{std::move(name)}
  {
  }

  std::string qualified_name::full(std::string const& delimiter) const
  {
    std::string result{name_};
    if (module_.empty()) {
      return result;
    }
    return module_ + delimiter + result;
  }

  bool qualified_name::operator==(qualified_name const& other) const
  {
    return std::tie(module_, name_) == std::tie(other.module_, other.name_);
  }

  bool qualified_name::operator!=(qualified_name const& other) const { return !operator==(other); }

  bool qualified_name::operator<(qualified_name const& other) const
  {
    return std::tie(module_, name_) < std::tie(other.module_, other.name_);
  }
}
