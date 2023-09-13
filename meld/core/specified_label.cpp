#include "meld/core/specified_label.hpp"

#include <stdexcept>
#include <tuple>

namespace meld {
  specified_label specified_label::operator()(std::string domain) &&
  {
    if (empty(domain)) {
      return {std::move(name)};
    }
    return {std::move(name), {std::move(domain)}};
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
}
