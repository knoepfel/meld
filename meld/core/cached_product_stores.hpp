#ifndef meld_core_cached_product_stores_hpp
#define meld_core_cached_product_stores_hpp

// FIXME: only intended to be used in a single-threaded context as std::map is not
//        thread-safe.

#include "meld/model/fwd.hpp"
#include "meld/model/product_store_factory.hpp"
#include "meld/model/transition.hpp"

namespace meld {

  class cached_product_stores {
  public:
    explicit cached_product_stores(product_store_factory factory) : factory_{std::move(factory)} {}
    product_store_ptr get_empty_store(level_id const& id, stage processing_stage = stage::process)
    {
      auto it = product_stores_.find(id);
      if (it != cend(product_stores_)) {
        return it->second;
      }
      if (id == level_id::base()) {
        return new_store(factory_.make(level_id::base(), source_name_));
      }
      return new_store(
        get_empty_store(id.parent())->make_child(id.back(), source_name_, processing_stage));
    }

  private:
    product_store_ptr new_store(product_store_ptr const& store)
    {
      return product_stores_.try_emplace(store->id(), store).first->second;
    }

    product_store_factory factory_;
    std::string const source_name_{"Source"};
    std::map<level_id, product_store_ptr> product_stores_;
  };

}

#endif /* meld_core_cached_product_stores_hpp */
