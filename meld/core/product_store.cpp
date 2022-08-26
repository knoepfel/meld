#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"

#include <memory>
#include <utility>

namespace meld {

  product_store::product_store(level_id id, action processing_action) :
    id_{std::move(id)}, action_{processing_action}
  {
  }

  product_store::product_store(std::shared_ptr<product_store> parent,
                               std::size_t new_level_number,
                               action processing_action) :
    parent_{parent}, id_{parent->id().make_child(new_level_number)}, action_{processing_action}
  {
  }

  product_store::product_store(std::shared_ptr<product_store> current, action processing_action) :
    parent_{current->parent()}, id_{current->id()}, action_{processing_action}
  {
  }

  product_store_ptr const&
  product_store::parent() const noexcept
  {
    return parent_;
  }

  product_store_ptr
  product_store::make_child(std::size_t new_level_number, action processing_action)
  {
    return std::make_shared<product_store>(shared_from_this(), new_level_number, processing_action);
  }

  product_store_ptr
  product_store::extend(action processing_action)
  {
    return std::make_shared<product_store>(shared_from_this(), processing_action);
  }

  level_id const&
  product_store::id() const noexcept
  {
    return id_;
  }

  std::size_t
  product_store::message_id() const noexcept
  {
    return id_.hash();
  }

  bool
  product_store::has(action const a) const noexcept
  {
    return action_ == a;
  }

  product_store_ptr
  make_product_store()
  {
    return std::make_shared<product_store>();
  }

  std::size_t
  ProductStoreHasher::operator()(product_store_ptr ptr) const noexcept
  {
    assert(ptr);
    return ptr->message_id();
  }
}
