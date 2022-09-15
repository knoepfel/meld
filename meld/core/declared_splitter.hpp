#ifndef meld_core_declared_splitter_hpp
#define meld_core_declared_splitter_hpp

#include "meld/core/concurrency.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"
#include "meld/core/product_store.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <concepts>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  class generator {
  public:
    explicit generator(product_store_ptr const& parent);
    void make_child(std::size_t i);

  private:
    product_store_ptr parent_;
  };

  class declared_splitter {
  public:
    declared_splitter(std::string name, std::size_t concurrency);

    virtual ~declared_splitter();

    std::string const& name() const noexcept;
    std::size_t concurrency() const noexcept;
    tbb::flow::receiver<message>& port(std::string const& product_name);
    virtual tbb::flow::sender<message>& sender() = 0;
    virtual std::span<std::string const, std::dynamic_extent> input() const = 0;

  private:
    virtual tbb::flow::receiver<message>& port_for(std::string const& product_name) = 0;

    std::string name_;
    std::size_t concurrency_;
  };

  using declared_splitter_ptr = std::unique_ptr<declared_splitter>;
  using declared_splitters = std::map<std::string, declared_splitter_ptr>;

  // Registering concrete splitters

  template <typename T, typename... Args>
  class incomplete_splitter {
    static constexpr auto N = sizeof...(Args);
    class complete_splitter;

  public:
    incomplete_splitter(user_functions<T>& funcs,
                        std::string name,
                        tbb::flow::graph& g,
                        void (*f)(generator&, Args...)) :
      funcs_{funcs}, name_{move(name)}, graph_{g}, ft_{f}
    {
    }

    // Icky?
    incomplete_splitter&
    concurrency(std::size_t n)
    {
      concurrency_ = n;
      return *this;
    }

    auto
    input(std::array<std::string, N> input_keys)
    {
      funcs_.add_splitter(name_,
                          std::make_unique<complete_splitter>(
                            name_, concurrency_, graph_, move(ft_), move(input_keys)));
    }

    template <typename... Ts>
    auto
    input(Ts... ts)
    {
      static_assert(std::conjunction_v<std::is_convertible<Ts, std::string>...>);
      static_assert(N == sizeof...(Ts),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return input(std::array<std::string, N>{ts...});
    }

  private:
    user_functions<T>& funcs_;
    std::string name_;
    std::size_t concurrency_{concurrency::serial};
    tbb::flow::graph& graph_;
    std::function<void(generator&, Args...)> ft_;
  };

  template <typename T, typename... Args>
  class incomplete_splitter<T, Args...>::complete_splitter : public declared_splitter {
    std::size_t
    port_index_for(std::string const& product_name)
    {
      auto it = std::find(cbegin(input_), cend(input_), product_name);
      if (it == cend(input_)) {
        throw std::runtime_error("Product name " + product_name + " not valid for splitter.");
      }
      return std::distance(cbegin(input_), it);
    }

    template <std::size_t I>
    tbb::flow::receiver<message>&
    receiver_for(std::size_t const index)
    {
      if constexpr (I < N) {
        if (I != index) {
          return receiver_for<I + 1ull>(index);
        }
        return input_port<I>(join_);
      }
      else {
        throw std::runtime_error("Should never get here");
      }
    }

    template <std::size_t... Is>
    void
    call(std::function<void(generator&, Args...)> ft,
         generator& g,
         messages_t<N> const& messages,
         std::index_sequence<Is...>)
    {
      return std::invoke(
        ft,
        g,
        std::get<Is>(messages).store->template get_handle<typename handle_for<Args>::value_type>(
          input_[Is])...);
    }

  public:
    complete_splitter(std::string name,
                      std::size_t concurrency,
                      tbb::flow::graph& g,
                      std::function<void(generator&, Args...)>&& f,
                      std::array<std::string, N> input) :
      declared_splitter{move(name), concurrency},
      input_{move(input)},
      join_{g, type_for_t<MessageHasher, Args>{}...},
      splitter_{
        g, concurrency, [this, ft = std::move(f)](messages_t<N> const& messages) -> message {
          auto const& msg = most_derived(messages);
          auto const& [store, message_id, _] = msg;
          if (store->is_flush()) {
            return msg;
          }

          if (typename decltype(stores_)::const_accessor a; stores_.find(a, store->id())) {
            return {a->second, message_id};
          }

          typename decltype(stores_)::accessor a;
          bool const new_insert = stores_.insert(a, store->id());
          if (!new_insert) {
            return {a->second, message_id};
          }

          generator g{msg.store};
          call(ft, g, messages, std::index_sequence_for<Args...>{});
          a->second = {};
          return {a->second, message_id};
        }}
    {
      if constexpr (N > 1ull) {
        make_edge(join_, splitter_);
      }
      else {
        make_edge(join_.pass_through, splitter_);
      }
    }

  private:
    tbb::flow::receiver<message>&
    port_for(std::string const& product_name) override
    {
      if constexpr (N > 1ull) {
        auto const index = port_index_for(product_name);
        return receiver_for<0ull>(index);
      }
      else {
        return join_.pass_through;
      }
    }

    tbb::flow::sender<message>&
    sender() override
    {
      return splitter_;
    }

    std::span<std::string const, std::dynamic_extent>
    input() const override
    {
      return input_;
    }

    std::array<std::string, N> input_;
    join_or_none_t<N> join_;
    tbb::flow::function_node<messages_t<N>, message> splitter_;
    tbb::concurrent_hash_map<level_id, product_store_ptr> stores_;
  };
}

#endif /* meld_core_declared_splitter_hpp */
