#include "meld/model/level_order.hpp"
#include "meld/model/level_hierarchy.hpp"

namespace meld {
  level_order::level_order() = default;

  level_order::level_order(level_hierarchy* hierarchy, std::vector<level_order_index> indices) :
    hierarchy_{hierarchy}, indices_{move(indices)}
  {
  }

  std::vector<std::string> level_order::to_strings() const
  {
    std::vector<std::string> result;
    result.reserve(indices_.size());
    transform(indices_.begin(), indices_.end(), back_inserter(result), [this](auto const i) {
      return hierarchy_->level_name(i);
    });
    return result;
  }

  std::size_t level_order::depth(std::string const& level_name) const
  {
    auto const b = indices_.begin();
    auto const e = indices_.end();
    auto it = find(b, e, hierarchy_->index(level_name));
    return it == e ? -1ull : distance(b, it);
  }

  std::string const& level_order::level_name(level_id const& id) const
  {
    auto const depth = id.depth();
    std::size_t hindex = depth > 0 ? indices_.back() : -1ull;
    return hierarchy_->level_name(hindex);
  }

  bool level_order::operator<(level_order const& other) const { return indices_ < other.indices_; }
}
