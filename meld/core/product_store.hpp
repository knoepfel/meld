#ifndef meld_core_product_store_hpp
#define meld_core_product_store_hpp

#include "meld/core/handle.hpp"
#include "meld/graph/transition.hpp"

#include <cstddef>
#include <map>
#include <memory>
#include <string>
#include <type_traits>

namespace meld {

  // template <typename T>
  // struct labeled_data {
  //   std::string label;
  //   T data;
  // };

  class product_store : public std::enable_shared_from_this<product_store> {
    using ptr = std::shared_ptr<product_store>;

  public:
    explicit product_store(level_id id = {},
                           std::string source = {},
                           stage processing_stage = stage::process);
    explicit product_store(ptr parent,
                           std::size_t new_level_number,
                           std::string source,
                           products new_products);
    explicit product_store(ptr parent,
                           std::size_t new_level_number,
                           std::string source,
                           stage processing_stage);

    // FIXME: 'stores_for_products()' may need to become a lazy range.
    std::map<std::string, std::weak_ptr<product_store>> stores_for_products();

    auto begin() const noexcept { return products_.begin(); }
    auto end() const noexcept { return products_.end(); }

    std::string const& source() const noexcept;
    ptr const& parent() const noexcept;
    ptr make_child(std::size_t new_level_number, std::string source, products new_products);
    ptr make_child(std::size_t new_level_number,
                   std::string source = {},
                   stage st = stage::process);
    level_id const& id() const noexcept;
    bool is_flush() const noexcept;

    template <typename T>
    T const& get_product(std::string const& key) const;

    template <typename T>
    handle<T> get_handle(std::string const& key) const;

    // Thread-unsafe operations
    template <typename T>
    void add_product(std::string const& key, T const& t);

    template <typename T>
    void add_product(std::string const& key, std::shared_ptr<T>&& t);

    // template <typename T>
    // void add_product(labeled_data<T>&& data);

  private:
    ptr parent_{nullptr};
    products products_{};
    level_id id_;
    std::string source_;
    stage stage_;
  };

  using product_store_ptr = std::shared_ptr<product_store>;

  product_store_ptr make_product_store(level_id id = {}, std::string source = {});

  product_store_ptr const& more_derived(product_store_ptr const& a, product_store_ptr const& b);

  template <std::size_t I, typename Tuple, typename Element>
  Element const& get_most_derived(Tuple const& tup, Element const& element)
  {
    constexpr auto N = std::tuple_size_v<Tuple>;
    if constexpr (I == N - 1) {
      return more_derived(element, std::get<I>(tup));
    }
    else {
      return get_most_derived<I + 1>(tup, more_derived(element, std::get<I>(tup)));
    }
  }

  template <typename Tuple>
  auto const& most_derived(Tuple const& tup)
  {
    constexpr auto N = std::tuple_size_v<Tuple>;
    static_assert(N > 0ull);
    if constexpr (N == 1ull) {
      return std::get<0>(tup);
    }
    else {
      return get_most_derived<1ull>(tup, std::get<0>(tup));
    }
  }

  // Implementation details
  template <typename T>
  void product_store::add_product(std::string const& key, T const& t)
  {
    add_product(key, std::make_shared<product<std::remove_cvref_t<T>>>(t));
  }

  template <typename T>
  void product_store::add_product(std::string const& key, std::shared_ptr<T>&& t)
  {
    products_.add(key, std::move(t));
  }

  // template <typename T>
  // void
  // product_store::add_product(labeled_data<T>&& ldata)
  // {
  //   add_product(ldata.label, std::forward<T>(ldata.data));
  // }

  template <typename T>
  [[nodiscard]] handle<T> product_store::get_handle(std::string const& key) const
  {
    return handle<T>{products_.get<T>(key), id_};
  }

  template <typename T>
  [[nodiscard]] T const& product_store::get_product(std::string const& key) const
  {
    return *get_handle<T>(key);
  }
}

#endif /* meld_core_product_store_hpp */
