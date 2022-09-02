#ifndef meld_core_product_store_hpp
#define meld_core_product_store_hpp

#include "meld/core/handle.hpp"
#include "meld/graph/transition.hpp"
#include "meld/utilities/demangle_symbol.hpp"
#include "meld/utilities/sized_tuple.hpp"

#include "oneapi/tbb/flow_graph.h" // <-- belongs somewhere else

#include <map>
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
    explicit product_store(level_id id = {},
                           stage processing_stage = stage::process,
                           std::size_t message_id = 0ull);
    // FIXME: Possible conflict with copy c'tor
    explicit product_store(product_store const& current, std::size_t message_id = -1ull);
    explicit product_store(std::shared_ptr<product_store> parent,
                           std::size_t new_level_number,
                           stage processing_stage,
                           std::size_t message_id);

    auto
    begin() const noexcept
    {
      return products_.begin();
    }
    auto
    end() const noexcept
    {
      return products_.end();
    }

    ptr const& parent() const noexcept;
    ptr store_with(std::string const& product_name, std::size_t message_id = -1ull);
    ptr make_child(std::size_t new_level_number,
                   stage st = stage::process,
                   std::size_t message_id = 0ull);
    ptr extend(std::size_t message_id = -1ull);
    level_id const& id() const noexcept;
    std::size_t message_id() const noexcept;
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

    template <typename T>
    void add_product(labeled_data<T>&& data);

  private:
    std::shared_ptr<product_store> parent_{nullptr};
    std::map<std::string, std::shared_ptr<product_base>> products_{};
    level_id id_;
    stage stage_;
    std::size_t message_id_;
  };

  using product_store_ptr = std::shared_ptr<product_store>;

  product_store_ptr make_product_store();

  template <std::size_t N>
  using stores_t = sized_tuple<product_store_ptr, N>;

  struct ProductStoreHasher {
    std::size_t operator()(product_store_ptr ptr) const noexcept;
  };

  product_store_ptr const& more_derived(product_store_ptr const& a, product_store_ptr const& b);

  template <std::size_t I, typename Tuple>
  product_store_ptr const&
  get_most_derived(Tuple const& tup, product_store_ptr const& store)
  {
    constexpr auto N = std::tuple_size_v<Tuple>;
    if constexpr (I == N - 1) {
      return more_derived(store, std::get<I>(tup));
    }
    else {
      return get_most_derived<I + 1>(tup, more_derived(store, std::get<I>(tup)));
    }
  }

  template <typename Tuple>
  product_store_ptr const&
  most_derived_store(Tuple const& tup)
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

  inline namespace put_somplace_else {
    template <std::size_t N>
    using join_product_stores_t = tbb::flow::join_node<stores_t<N>, tbb::flow::tag_matching>;

    struct no_join {
      no_join(tbb::flow::graph& g, ProductStoreHasher) :
        pass_through{
          g, tbb::flow::unlimited, [](product_store_ptr const& store) { return std::tuple{store}; }}
      {
      }
      tbb::flow::function_node<product_store_ptr, stores_t<1ull>> pass_through;
    };
  }

  template <std::size_t N>
  using join_or_none_t = std::conditional_t<N == 1ull, no_join, join_product_stores_t<N>>;

  // Implementation details
  template <typename T>
  void
  product_store::add_product(std::string const& key, T const& t)
  {
    add_product(key, std::make_shared<product<std::remove_cvref_t<T>>>(t, message_id_));
  }

  template <typename T>
  void
  product_store::add_product(std::string const& key, std::shared_ptr<T>&& t)
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
      return handle<T>{"No product exists with the key '" + key + "'.", id_};
    }
    if (auto t = dynamic_cast<product<T> const*>(it->second.get())) {
      return handle<T>{*t, id_};
    }
    return handle<T>{
      "Cannot get product '" + key + "' with type '" + demangle_symbol(typeid(T)) + "'.", id_};
  }

  template <typename T>
  [[nodiscard]] T const&
  product_store::get_product(std::string const& key) const
  {
    return *get_handle<T>(key);
  }
}

#endif /* meld_core_product_store_hpp */
