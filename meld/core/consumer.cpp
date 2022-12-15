#include "meld/core/consumer.hpp"

namespace meld {
  consumer::consumer(std::string name, std::vector<std::string> preceding_filters) :
    name_{move(name)}, preceding_filters_{move(preceding_filters)}
  {
  }

  std::string const& consumer::name() const noexcept { return name_; }

  std::vector<std::string> const& consumer::filtered_by() const noexcept
  {
    return preceding_filters_;
  }
}
