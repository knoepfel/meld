#include "meld/model/product_store_factory.hpp"
#include "meld/model/level_hierarchy.hpp"
#include "meld/model/product_store.hpp"

namespace meld {
  product_store_factory::product_store_factory(level_hierarchy* hierarchy, level_order order) :
    hierarchy_{hierarchy}, order_{std::move(order)}
  {
  }

  std::size_t product_store_factory::depth(std::string const& level_name) const
  {
    return order_.depth(level_name);
  }

  std::string const& product_store_factory::level_name(level_id const& id) const
  {
    return order_.level_name(id);
  }

  product_store_ptr product_store_factory::make(level_id id,
                                                std::string_view source,
                                                stage processing_stage) const
  {
    auto const depth = id.depth();
    // The bookkeeping model is that each stage::flush ID is one slot deeper than the
    // corresponding stage::process ID--this extra slot specifies the number of sublevels
    // processed within the level.
    auto const adjustment = static_cast<std::size_t>(processing_stage == stage::flush);
    if (depth > order_.size() + adjustment) {
      throw std::runtime_error("Requested store would have an ID with a depth greater than what "
                               "is supported by this factory.");
    }
    return std::make_shared<product_store>(this, std::move(id), source, processing_stage);
  }

  product_store_factory product_store_factory::extend(std::string next_level) const
  {
    auto levels = order_.to_strings();
    levels.push_back(move(next_level));
    return hierarchy_->make_factory(move(levels));
  }
}
