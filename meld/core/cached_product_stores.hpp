#ifndef meld_core_cached_product_stores_hpp
#define meld_core_cached_product_stores_hpp

#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"

namespace meld {

  class cached_product_stores {
  public:
    product_store_ptr
    get_store(level_id const& id, bool is_flush = false)
    {
      auto it = product_stores_.find(id);
      if (it != cend(product_stores_)) {
        return it->second;
      }
      if (id == level_id{}) {
        return new_store(std::make_shared<product_store>(id));
      }
      return new_store(get_store(id.parent())->make_child(id.back(), false));
    }

  private:
    product_store_ptr
    new_store(std::shared_ptr<product_store> const& store)
    {
      return product_stores_.try_emplace(store->id(), store).first->second;
    }

    std::map<level_id, product_store_ptr> product_stores_;
  };

}

#endif /* meld_core_cached_product_stores_hpp */
