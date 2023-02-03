#include "meld/model/product_store.hpp"
#include "meld/model/level_hierarchy.hpp"
#include "meld/model/level_id.hpp"

#include <memory>
#include <utility>

namespace meld {

  product_store::product_store(level_id_ptr id, std::string_view source, stage processing_stage) :
    id_{move(id)}, source_{source}, stage_{processing_stage}
  {
  }

  product_store::product_store(product_store_ptr parent,
                               std::size_t new_level_number,
                               std::string const& new_level_name,
                               std::string_view source,
                               products new_products) :
    parent_{parent},
    products_{std::move(new_products)},
    id_{parent->id()->make_child(new_level_number, new_level_name)},
    source_{source},
    stage_{stage::process}
  {
  }

  product_store::product_store(product_store_ptr parent,
                               std::size_t new_level_number,
                               std::string const& new_level_name,
                               std::string_view source,
                               stage processing_stage) :
    parent_{parent},
    id_{parent->id()->make_child(new_level_number, new_level_name)},
    source_{source},
    stage_{processing_stage}
  {
  }

  product_store_ptr product_store::base() { return ptr{new product_store}; }

  product_store_const_ptr product_store::parent(std::string const& level_name) const
  {
    auto store = parent_;
    while (store != nullptr) {
      if (store->level_name() == level_name) {
        return store;
      }
      store = store->parent_;
    }
    return nullptr;
  }

  product_store_const_ptr product_store::store_for_product(std::string const& product_name) const
  {
    auto store = shared_from_this();
    while (store != nullptr) {
      if (store->contains_product(product_name)) {
        return store;
      }
      store = store->parent_;
    }
    return nullptr;
  }

  product_store_ptr product_store::make_flush(level_id_ptr id) const
  {
    return ptr{new product_store{std::move(id), "[inserted]", stage::flush}};
  }

  product_store_ptr product_store::make_parent(std::string_view source) const
  {
    return ptr{new product_store{id_->parent(), source, stage::process}};
  }

  product_store_ptr product_store::make_parent(std::string const& level_name,
                                               std::string_view source) const
  {
    auto parent_id = id_->parent(level_name);
    if (!parent_id) {
      throw std::runtime_error("Trying to create parent with non-existent level " + level_name);
    }
    return ptr{new product_store{parent_id, source, stage::process}};
  }

  product_store_ptr product_store::make_continuation(std::string_view source) const
  {
    return ptr{new product_store{id_, source, stage::process}};
  }

  product_store_ptr product_store::make_continuation(level_id_ptr id, std::string_view source) const
  {
    return ptr{new product_store{move(id), source, stage::process}};
  }

  product_store_ptr product_store::make_child(std::size_t new_level_number,
                                              std::string const& new_level_name,
                                              std::string_view source,
                                              products new_products)
  {
    return ptr{new product_store{
      shared_from_this(), new_level_number, new_level_name, source, std::move(new_products)}};
  }

  product_store_ptr product_store::make_child(std::size_t new_level_number,
                                              std::string const& new_level_name,
                                              std::string_view source,
                                              stage processing_stage)
  {
    return ptr{new product_store{
      shared_from_this(), new_level_number, new_level_name, source, processing_stage}};
  }

  std::string const& product_store::level_name() const noexcept { return id_->level_name(); }
  std::string_view product_store::source() const noexcept { return source_; }
  product_store_ptr const& product_store::parent() const noexcept { return parent_; }
  level_id_ptr const& product_store::id() const noexcept { return id_; }
  bool product_store::is_flush() const noexcept { return stage_ == stage::flush; }

  bool product_store::contains_product(std::string const& product_name) const
  {
    return products_.contains(product_name);
  }

  product_store_ptr const& more_derived(product_store_ptr const& a, product_store_ptr const& b)
  {
    if (a->id()->depth() > b->id()->depth()) {
      return a;
    }
    return b;
  }
}
