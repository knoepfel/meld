#include "meld/core/message.hpp"
#include "meld/model/level_id.hpp"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <tuple>

namespace meld {

  std::size_t MessageHasher::operator()(message const& msg) const noexcept { return msg.id; }

  message const& more_derived(message const& a, message const& b)
  {
    if (a.store->id()->depth() > b.store->id()->depth()) {
      return a;
    }
    return b;
  }

  std::size_t port_index_for(std::span<specified_label const> product_names,
                             std::string const& product_name)
  {
    auto const [b, e] = std::make_tuple(cbegin(product_names), cend(product_names));
    auto matcher = [&product_name](auto const& label) { return label.name == product_name; };
    auto result = count_if(b, e, matcher);
    if (result == 0) {
      throw std::runtime_error("Algorithm does not accept product '" + product_name + "'.");
    }
    else if (result > 1) {
      throw std::runtime_error("Algorithm uses product '" + product_name + "' more than once.");
    }
    return distance(b, find_if(b, e, matcher));
  }
}
