#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"

#include <memory>
#include <utility>

namespace meld {

  product_store::product_store(level_id id, stage processing_stage) :
    id_{std::move(id)}, stage_{processing_stage}
  {
  }

  product_store::product_store(product_store_ptr parent,
                               std::size_t new_level_number,
                               stage processing_stage) :
    parent_{parent}, id_{parent->id().make_child(new_level_number)}, stage_{processing_stage}
  {
  }

  std::map<std::string, std::weak_ptr<product_store>>
  product_store::stores_for_products()
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

  product_store_ptr const&
  product_store::parent() const noexcept
  {
    return parent_;
  }

  product_store_ptr
  product_store::make_child(std::size_t new_level_number, stage processing_stage)
  {
    return std::make_shared<product_store>(shared_from_this(), new_level_number, processing_stage);
  }

  level_id const&
  product_store::id() const noexcept
  {
    return id_;
  }

  bool
  product_store::is_flush() const noexcept
  {
    return stage_ == stage::flush;
  }

  product_store_ptr
  make_product_store()
  {
    return std::make_shared<product_store>();
  }

  std::size_t
  MessageHasher::operator()(message const& msg) const noexcept
  {
    return msg.id;
  }

  product_store_ptr const&
  more_derived(product_store_ptr const& a, product_store_ptr const& b)
  {
    if (a->id().depth() > b->id().depth()) {
      return a;
    }
    return b;
  }

  message const&
  more_derived(message const& a, message const& b)
  {
    if (a.store->id().depth() > b.store->id().depth()) {
      return a;
    }
    return b;
  }
}
