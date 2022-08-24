#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"

#include <memory>
#include <utility>

namespace meld {

  product_store::product_store(level_id id, bool is_flush) : id_{std::move(id)}, is_flush_{is_flush}
  {
  }

  product_store::product_store(std::shared_ptr<product_store> parent,
                               std::size_t new_level_number,
                               bool is_flush) :
    parent_{parent}, id_{parent->id().make_child(new_level_number)}, is_flush_{is_flush}
  {
  }
  product_store_ptr
  product_store::parent() const noexcept
  {
    return parent_;
  }

  product_store_ptr
  product_store::make_child(std::size_t new_level_number, bool is_flush)
  {
    return std::make_shared<product_store>(shared_from_this(), new_level_number, is_flush);
  }

  level_id const&
  product_store::id() const noexcept
  {
    return id_;
  }

  bool
  product_store::is_flush() const noexcept
  {
    return is_flush_;
  }

  product_store_ptr
  make_product_store()
  {
    return std::make_shared<product_store>();
  }

}
