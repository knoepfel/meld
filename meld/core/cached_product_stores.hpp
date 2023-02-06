#ifndef meld_core_cached_product_stores_hpp
#define meld_core_cached_product_stores_hpp

// FIXME: only intended to be used in a single-threaded context as std::map is not
//        thread-safe.

#include "meld/model/fwd.hpp"
#include "meld/model/level_id.hpp"
#include "meld/model/product_store.hpp"

namespace meld {

  class cached_product_stores {
  public:
    product_store_ptr get_store(level_id_ptr id, stage processing_stage = stage::process)
    {
      auto it = product_stores_.find(id->hash());
      if (it != cend(product_stores_)) {
        return it->second;
      }
      if (id == level_id::base_ptr()) {
        return new_store(product_store::base());
      }
      return new_store(
        get_store(id->parent())->make_child(id->number(), "", source_name_, processing_stage));
    }

  private:
    product_store_ptr new_store(product_store_ptr const& store)
    {
      return product_stores_.try_emplace(store->id()->hash(), store).first->second;
    }

    std::string const source_name_{"Source"};
    std::map<level_id::hash_type, product_store_ptr> product_stores_{};
  };

}

#endif /* meld_core_cached_product_stores_hpp */
