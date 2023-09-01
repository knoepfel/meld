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
    auto it =
      find_if(b, e, [&product_name](auto const& label) { return label.name == product_name; });
    if (it == e) {
      throw std::runtime_error("Product name " + product_name + " not valid for splitter.");
    }
    return distance(b, it);
  }
}
