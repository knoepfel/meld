#include "meld/core/specified_label.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include <stdexcept>
#include <tuple>

namespace meld {
  specified_label specified_label::operator()(std::string domain) &&
  {
    return {std::move(name), std::move(domain)};
  }

  specified_label specified_label::related_to(std::string relation) &&
  {
    return {std::move(name),
            std::move(domain),
            std::make_shared<specified_label>(specified_label{relation})};
  }

  specified_label specified_label::related_to(specified_label relation) &&
  {
    return {
      std::move(name), std::move(domain), std::make_shared<specified_label>(std::move(relation))};
  }

  std::string specified_label::to_string() const
  {
    if (domain.empty()) {
      return fmt::format("{} ϵ (any)", name);
    }
    return fmt::format("{} ϵ {}", name, domain);
  }

  specified_label operator""_in(char const* name, std::size_t length)
  {
    if (length == 0ull) {
      throw std::runtime_error("Cannot specify product with empty name.");
    }
    return {name};
  }

  bool operator==(specified_label const& a, specified_label const& b)
  {
    return std::tie(a.name, a.domain) == std::tie(b.name, b.domain);
  }

  bool operator!=(specified_label const& a, specified_label const& b) { return !(a == b); }

  bool operator<(specified_label const& a, specified_label const& b)
  {
    return std::tie(a.name, a.domain) < std::tie(b.name, b.domain);
  }

  std::ostream& operator<<(std::ostream& os, specified_label const& label)
  {
    os << label.to_string();
    return os;
  }
}
