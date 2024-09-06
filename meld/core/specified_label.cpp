#include "meld/core/specified_label.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include <stdexcept>
#include <tuple>

namespace meld {
  specified_label specified_label::operator()(std::string family) &&
  {
    return {std::move(name), std::move(family)};
  }

  specified_label specified_label::related_to(std::string relation) &&
  {
    return {std::move(name),
            std::move(family),
            std::make_shared<specified_label>(specified_label{relation})};
  }

  specified_label specified_label::related_to(specified_label relation) &&
  {
    return {
      std::move(name), std::move(family), std::make_shared<specified_label>(std::move(relation))};
  }

  std::string specified_label::to_string() const
  {
    if (family.empty()) {
      return name.full();
    }
    return fmt::format("{} ϵ {}", name.full(), family);
  }

  specified_label specified_label::create(char const* c) { return create(std::string{c}); }

  specified_label specified_label::create(std::string const& s)
  {
    auto pos = s.find("/");
    if (pos == std::string::npos) {
      return {qualified_name{s}};
    }
    return {qualified_name{s.substr(0, pos), s.substr(pos + 1)}};
  }

  specified_label specified_label::create(specified_label l) { return l; }

  specified_label operator""_in(char const* name, std::size_t length)
  {
    if (length == 0ull) {
      throw std::runtime_error("Cannot specify product with empty name.");
    }
    return {name};
  }

  bool operator==(specified_label const& a, specified_label const& b)
  {
    return std::tie(a.name, a.family) == std::tie(b.name, b.family);
  }

  bool operator!=(specified_label const& a, specified_label const& b) { return !(a == b); }

  bool operator<(specified_label const& a, specified_label const& b)
  {
    return std::tie(a.name, a.family) < std::tie(b.name, b.family);
  }

  std::ostream& operator<<(std::ostream& os, specified_label const& label)
  {
    os << label.to_string();
    return os;
  }
}
