#ifndef meld_model_level_order_hpp
#define meld_model_level_order_hpp

#include "meld/model/fwd.hpp"

#include <cstddef>
#include <vector>

namespace meld {

  using level_order_index = std::size_t;
  class level_order {
  public:
    level_order();
    level_order(level_hierarchy* hiearchy, std::vector<level_order_index> indices);
    std::size_t size() const noexcept { return indices_.size(); }

    std::string const& level_name(level_id const& id) const;
    std::size_t depth(std::string const& level_name) const;
    std::vector<std::string> to_strings() const;
    bool operator<(level_order const& other) const;

  private:
    level_hierarchy* hierarchy_{nullptr};
    std::vector<level_order_index> indices_{};
  };

}

#endif /* meld_model_level_order_hpp */
