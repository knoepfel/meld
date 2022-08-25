#ifndef meld_core_product_store_hpp
#define meld_core_product_store_hpp

#include "meld/core/handle.hpp"
#include "meld/graph/transition.hpp"
#include "meld/utilities/demangle_symbol.hpp"

#include "oneapi/tbb/concurrent_unordered_map.h"

#include <memory>
#include <string>

namespace meld {

  template <typename T>
  struct labeled_data {
    std::string label;
    T data;
  };

  class product_store : public std::enable_shared_from_this<product_store> {
    using ptr = std::shared_ptr<product_store>;

  public:
    explicit product_store(level_id id = {}, bool is_flush = false);
    explicit product_store(std::shared_ptr<product_store> parent,
                           std::size_t new_level_number,
                           bool is_flush);

    ptr parent() const noexcept;
    ptr make_child(std::size_t new_level_number, bool is_flush);
    level_id const& id() const noexcept;
    bool is_flush() const noexcept;

    template <typename T>
    void add_product(std::string const& key, T const& t);

    template <typename T>
    void add_product(std::string const& key, std::unique_ptr<T>&& t);

    template <typename T>
    void add_product(labeled_data<T>&& data);

    template <typename T>
    T const& get_product(std::string const& key) const;

    template <typename T>
    handle<T> get_handle(std::string const& key) const;

  private:
    std::shared_ptr<product_store> parent_{nullptr};
    tbb::concurrent_unordered_map<std::string, std::unique_ptr<product_base>> products_{};
    level_id id_;
    bool is_flush_;
  };

  using product_store_ptr = std::shared_ptr<product_store>;

  product_store_ptr make_product_store();

  // Implementation details
  template <typename T>
  void
  product_store::add_product(std::string const& key, T const& t)
  {
    add_product(key, std::make_unique<product<std::remove_cvref_t<T>>>(t));
  }

  template <typename T>
  void
  product_store::add_product(std::string const& key, std::unique_ptr<T>&& t)
  {
    products_.emplace(key, std::move(t));
  }

  template <typename T>
  void
  product_store::add_product(labeled_data<T>&& ldata)
  {
    add_product(ldata.label, std::forward<T>(ldata.data));
  }

  template <typename T>
  [[nodiscard]] handle<T>
  product_store::get_handle(std::string const& key) const
  {
    auto it = products_.find(key);
    if (it == cend(products_)) {
      return handle<T>{"No product exists with the key '" + key + "'."};
    }
    if (auto t = dynamic_cast<product<T> const*>(it->second.get())) {
      return handle<T>{*t};
    }
    return handle<T>{"Cannot get product '" + key + "' with type '" + demangle_symbol(typeid(T)) +
                     "'."};
  }

  template <typename T>
  [[nodiscard]] T const&
  product_store::get_product(std::string const& key) const
  {
    return *get_handle<T>(key);
  }

  struct ProductStoreHasher {
    auto
    operator()(product_store_ptr ptr) const
    {
      assert(ptr);
      return ptr->id().hash();
    }
  };
}

#endif /* meld_core_product_store_hpp */
