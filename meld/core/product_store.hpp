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
    explicit product_store(level_id id = {}, stage processing_stage = stage::process);
    explicit product_store(ptr parent, std::size_t new_level_number, stage processing_stage);

    // FIXME: 'stores_for_products()' may need to become a lazy range.
    std::map<std::string, std::weak_ptr<product_store>> stores_for_products();

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
    ptr make_child(std::size_t new_level_number, stage st = stage::process);
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

    template <typename T>
    void add_product(labeled_data<T>&& data);

  private:
    ptr parent_{nullptr};
    std::map<std::string, std::shared_ptr<product_base>> products_{};
    level_id id_;
    stage stage_;
  };

  using product_store_ptr = std::shared_ptr<product_store>;

  product_store_ptr make_product_store();

  struct message {
    product_store_ptr store;
    std::size_t id;
    std::size_t original_id; // Used during flush
  };

  template <std::size_t N>
  using messages_t = sized_tuple<message, N>;

  struct MessageHasher {
    std::size_t operator()(message const& msg) const noexcept;
  };

  product_store_ptr const& more_derived(product_store_ptr const& a, product_store_ptr const& b);
  message const& more_derived(message const& a, message const& b);

  template <std::size_t I, typename Tuple, typename Element>
  Element const&
  get_most_derived(Tuple const& tup, Element const& element)
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
  auto const&
  most_derived(Tuple const& tup)
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
    using join_messages_t = tbb::flow::join_node<messages_t<N>, tbb::flow::tag_matching>;

    struct no_join {
      no_join(tbb::flow::graph& g, MessageHasher) :
        pass_through{g, tbb::flow::unlimited, [](message const& msg) { return std::tuple{msg}; }}
      {
      }
      tbb::flow::function_node<message, messages_t<1ull>> pass_through;
    };
  }

  template <std::size_t N>
  using join_or_none_t = std::conditional_t<N == 1ull, no_join, join_messages_t<N>>;

  // Implementation details
  template <typename T>
  void
  product_store::add_product(std::string const& key, T const& t)
  {
    add_product(key, std::make_shared<product<std::remove_cvref_t<T>>>(t));
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
