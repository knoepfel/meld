#include "meld/core/consumer.hpp"

namespace meld {
  consumer::consumer(std::string name,
                     std::vector<std::string> preceding_filters,
                     std::vector<std::string> receive_stores) :
    name_{std::move(name)},
    preceding_filters_{std::move(preceding_filters)},
    receive_stores_{std::move(receive_stores)}
  {
  }

  std::string const& consumer::name() const noexcept { return name_; }

  std::vector<std::string> const& consumer::filtered_by() const noexcept
  {
    return preceding_filters_;
  }

  std::vector<std::string> const& consumer::receive_stores() const noexcept
  {
    return receive_stores_;
  }
}
