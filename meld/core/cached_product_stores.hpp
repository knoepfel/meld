#ifndef meld_core_cached_product_stores_hpp
#define meld_core_cached_product_stores_hpp

#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"

#include <atomic>

namespace meld {

  class cached_product_stores {
  public:
    product_store_ptr
    get_store(level_id const& id, stage processing_stage = stage::process)
    {
      auto it = product_stores_.find(id);
      if (it != cend(product_stores_)) {
        return it->second;
      }
      if (id == level_id{}) {
        return new_store(std::make_shared<product_store>());
      }
      ++store_counter_;
      return new_store(
        get_store(id.parent())->make_child(id.back(), processing_stage, store_counter_.load()));
    }

  private:
    product_store_ptr
    new_store(std::shared_ptr<product_store> const& store)
    {
      return product_stores_.try_emplace(store->id(), store).first->second;
    }

    std::map<level_id, product_store_ptr> product_stores_;
    std::atomic<std::size_t> store_counter_{};
  };

}

#endif /* meld_core_cached_product_stores_hpp */
