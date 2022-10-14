#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"

#include <memory>
#include <utility>

namespace meld {

  product_store::product_store(level_id id, std::string source, stage processing_stage) :
    id_{std::move(id)}, source_{move(source)}, stage_{processing_stage}
  {
  }

  product_store::product_store(product_store_ptr parent,
                               std::size_t new_level_number,
                               std::string source,
                               products new_products) :
    parent_{parent},
    products_{std::move(new_products)},
    id_{parent->id().make_child(new_level_number)},
    source_{move(source)},
    stage_{stage::process}
  {
  }

  product_store::product_store(product_store_ptr parent,
                               std::size_t new_level_number,
                               std::string source,
                               stage processing_stage) :
    parent_{parent},
    id_{parent->id().make_child(new_level_number)},
    source_{move(source)},
    stage_{processing_stage}
  {
  }

  std::map<std::string, std::weak_ptr<product_store>> product_store::stores_for_products()
  {
    std::map<std::string, std::weak_ptr<product_store>> result;
    auto store = shared_from_this();
    while (store != nullptr) {
      for (auto const& [key, _] : *store) {
        result.try_emplace(key, store);
      }
      store = store->parent_;
    }
    return result;
  }

  product_store_ptr product_store::make_child(std::size_t new_level_number,
                                              std::string source,
                                              products new_products)
  {
    return std::make_shared<product_store>(
      shared_from_this(), new_level_number, move(source), std::move(new_products));
  }

  product_store_ptr product_store::make_child(std::size_t new_level_number,
                                              std::string source,
                                              stage processing_stage)
  {
    return std::make_shared<product_store>(
      shared_from_this(), new_level_number, move(source), processing_stage);
  }

  std::string const& product_store::source() const noexcept { return source_; }
  product_store_ptr const& product_store::parent() const noexcept { return parent_; }
  level_id const& product_store::id() const noexcept { return id_; }
  bool product_store::is_flush() const noexcept { return stage_ == stage::flush; }

  bool product_store::contains_product(std::string const& product_name) const
  {
    return products_.contains(product_name);
  }

  product_store_ptr make_product_store(level_id id, std::string source, stage processing_stage)
  {
    return std::make_shared<product_store>(std::move(id), move(source), processing_stage);
  }

  product_store_ptr const& more_derived(product_store_ptr const& a, product_store_ptr const& b)
  {
    if (a->id().depth() > b->id().depth()) {
      return a;
    }
    return b;
  }
}
