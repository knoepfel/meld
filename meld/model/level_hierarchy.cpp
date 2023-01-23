#include "meld/model/level_hierarchy.hpp"
#include "meld/model/product_store_factory.hpp"

#include <ostream>

namespace {
  std::string const root_name{"(root)"};
}

namespace meld {

  product_store_factory level_hierarchy::make_factory(std::vector<std::string> levels)
  {
    return product_store_factory{this, add_order(levels)};
  }

  std::size_t level_hierarchy::index(std::string const& name) const
  {
    auto const b = begin(levels_);
    auto const e = end(levels_);
    auto it = find_if(b, e, [&name](auto const& level) { return level.name == name; });
    return it != e ? distance(b, it) : -1ull;
  }

  std::string const& level_hierarchy::level_name(level_order_index hindex) const
  {
    if (hindex == -1ull) {
      return root_name;
    }
    return levels_[hindex].name;
  }

  std::vector<std::vector<std::string>> level_hierarchy::orders() const
  {
    std::vector<std::vector<std::string>> result;
    result.reserve(size(orders_));
    for (auto const& order : orders_) {
      result.push_back(order.to_strings());
    }
    return result;
  }

  void level_hierarchy::print(std::ostream& os) const
  {
    for (auto const& order : orders_) {
      print(os, order);
    }
  }

  level_order level_hierarchy::add_order(std::vector<std::string> const& levels)
  {
    if (empty(levels))
      return {};

    std::vector<level_order_index> indices;
    indices.reserve(size(levels));
    for (std::size_t local_parent_index = -1ull; auto const& level : levels) {
      auto i = index(level);
      if (i != -1ull) {
        indices.push_back(i);
        ++local_parent_index;
        continue;
      }

      std::size_t parent_index = local_parent_index;
      if (local_parent_index != -1ull) {
        parent_index = index(levels[local_parent_index]);
      }
      indices.push_back(size(levels_));
      levels_.push_back({level, parent_index});
      ++local_parent_index;
    }
    return *orders_.emplace(this, std::move(indices)).first;
  }

  void level_hierarchy::print(std::ostream& os, level_order const& h) const
  {
    os << "[";
    for (std::string prefix; auto const& name : h.to_strings()) {
      os << prefix << '"' << name << '"';
      if (empty(prefix)) {
        prefix = ", ";
      }
    }
    os << "]\n";
  }

}
