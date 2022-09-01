#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"

#include <memory>
#include <utility>

namespace meld {

  product_store::product_store(level_id id, stage processing_stage, std::size_t message_id) :
    id_{std::move(id)}, stage_{processing_stage}, message_id_{message_id}
  {
  }

  product_store::product_store(std::shared_ptr<product_store> parent,
                               std::size_t new_level_number,
                               stage processing_stage,
                               std::size_t message_id) :
    parent_{parent},
    id_{parent->id().make_child(new_level_number)},
    stage_{processing_stage},
    message_id_{message_id}
  {
  }

  product_store::product_store(product_store const& current, std::size_t message_id) :
    parent_{current.parent_},
    id_{current.id_},
    stage_{current.stage_},
    message_id_{message_id == -1ull ? current.message_id() : message_id}
  {
  }

  product_store_ptr const&
  product_store::parent() const noexcept
  {
    return parent_;
  }

  product_store_ptr
  product_store::store_with(std::string const& product_name, std::size_t message_id)
  {
    if (message_id == -1ull) {
      message_id = message_id_;
    }
    if (products_.contains(product_name)) {
      return extend(message_id);
    }
    if (parent_) {
      return parent_->store_with(product_name, message_id);
    }
    throw std::runtime_error("Store does not contain product with the name '" + product_name +
                             "'.");
  }

  product_store_ptr
  product_store::make_child(std::size_t new_level_number,
                            stage processing_stage,
                            std::size_t message_id)
  {
    return std::make_shared<product_store>(
      shared_from_this(), new_level_number, processing_stage, message_id);
  }

  product_store_ptr
  product_store::extend(std::size_t message_id)
  {
    return std::make_shared<product_store>(*this, message_id);
  }

  level_id const&
  product_store::id() const noexcept
  {
    return id_;
  }

  std::size_t
  product_store::message_id() const noexcept
  {
    return message_id_;
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
  ProductStoreHasher::operator()(product_store_ptr ptr) const noexcept
  {
    assert(ptr);
    return ptr->message_id();
  }

  product_store_ptr const&
  more_derived(product_store_ptr const& a, product_store_ptr const& b)
  {
    if (a->id().depth() > b->id().depth()) {
      return a;
    }
    return b;
  }
}
