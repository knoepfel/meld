#include "meld/model/product_store.hpp"
#include "meld/model/level_hierarchy.hpp"
#include "meld/model/transition.hpp"

#include <memory>
#include <utility>

namespace meld {

  product_store::product_store(product_store_factory const* factory,
                               level_id id,
                               std::string_view source,
                               stage processing_stage) :
    factory_{factory}, id_{std::move(id)}, source_{source}, stage_{processing_stage}
  {
  }

  product_store::product_store(product_store_ptr parent,
                               std::size_t new_level_number,
                               std::string_view source,
                               products new_products) :
    factory_{parent->factory_},
    parent_{parent},
    products_{std::move(new_products)},
    id_{parent->id().make_child(new_level_number)},
    source_{source},
    stage_{stage::process}
  {
  }

  product_store::product_store(product_store_ptr parent,
                               std::size_t new_level_number,
                               std::string_view source,
                               stage processing_stage) :
    factory_{parent->factory_},
    parent_{parent},
    id_{parent->id().make_child(new_level_number)},
    source_{source},
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

  product_store_ptr product_store::make_flush(level_id const& id) const
  {
    return factory_->make(id, "[inserted]", stage::flush);
  }

  product_store_ptr product_store::make_parent(std::string_view source) const
  {
    return factory_->make(id_.parent(), source, stage::process);
  }

  product_store_ptr product_store::make_parent(std::string const& level_name,
                                               std::string_view source) const
  {
    auto depth = factory_->depth(level_name);
    if (depth == -1ull) {
      throw std::runtime_error("Trying to create parent with non-existent level " + level_name);
    }
    return factory_->make(id_.parent(depth), source, stage::process);
  }

  product_store_ptr product_store::make_continuation(std::string_view source) const
  {
    return factory_->make(id_, source, stage::process);
  }

  product_store_ptr product_store::make_child(std::size_t new_level_number,
                                              std::string_view source,
                                              products new_products)
  {
    return std::make_shared<product_store>(
      shared_from_this(), new_level_number, source, std::move(new_products));
  }

  product_store_ptr product_store::make_child(std::size_t new_level_number,
                                              std::string_view source,
                                              stage processing_stage)
  {
    return std::make_shared<product_store>(
      shared_from_this(), new_level_number, source, processing_stage);
  }

  std::string const& product_store::level_name() const noexcept
  {
    return factory_->level_name(id_);
  }
  std::string_view product_store::source() const noexcept { return source_; }
  product_store_ptr const& product_store::parent() const noexcept { return parent_; }
  level_id const& product_store::id() const noexcept { return id_; }
  bool product_store::is_flush() const noexcept { return stage_ == stage::flush; }

  bool product_store::contains_product(std::string const& product_name) const
  {
    return products_.contains(product_name);
  }

  product_store_ptr const& more_derived(product_store_ptr const& a, product_store_ptr const& b)
  {
    if (a->id().depth() > b->id().depth()) {
      return a;
    }
    return b;
  }
}
