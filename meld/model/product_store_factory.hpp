#ifndef meld_model_product_store_factory_hpp
#define meld_model_product_store_factory_hpp

#include "meld/model/fwd.hpp"
#include "meld/model/level_order.hpp"
#include "meld/model/product_store.hpp"

namespace meld {

  class product_store_factory {
  public:
    explicit product_store_factory(level_hierarchy* hierarchy, level_order order);

    std::size_t depth(std::string const& level_name) const;
    std::string const& level_name(level_id const& id) const;
    product_store_factory extend(std::string next_level) const;
    product_store_ptr make(level_id id = {},
                           std::string_view source = {},
                           stage processing_stage = stage::process) const;

  private:
    level_hierarchy* hierarchy_;
    level_order order_;
  };
}

#endif /* meld_model_product_store_factory_hpp */
