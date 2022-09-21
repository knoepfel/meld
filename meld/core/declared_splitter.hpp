#ifndef meld_core_declared_splitter_hpp
#define meld_core_declared_splitter_hpp

#include "meld/core/concurrency.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"
#include "meld/core/multiplexer.hpp"
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
    explicit generator(product_store_ptr const& parent,
                       multiplexer& m,
                       std::atomic<std::size_t>& counter);
    void make_child(std::size_t i, products new_products = {});
    message flush_message();

  private:
    product_store_ptr parent_;
    multiplexer& multiplexer_;
    std::atomic<std::size_t>& counter_;
    std::atomic<std::size_t> calls_{};
    std::size_t const original_message_id_{counter_};
  };

  class declared_splitter {
  public:
    declared_splitter(std::string name, std::size_t concurrency);

    virtual ~declared_splitter();

    std::string const& name() const noexcept;
    std::size_t concurrency() const noexcept;
    tbb::flow::receiver<message>& port(std::string const& product_name);
    virtual std::span<std::string const, std::dynamic_extent> input() const = 0;
    virtual std::vector<std::string> const& provided_products() const = 0;
    virtual void finalize(multiplexer::head_nodes_t head_nodes) = 0;

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

    // Icky?
    incomplete_splitter&
    input(std::array<std::string, N> input_keys)
    {
      input_keys_ = move(input_keys);
      return *this;
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

    auto
    provides(std::vector<std::string> product_names)
    {
      funcs_.add_splitter(
        name_,
        std::make_unique<complete_splitter>(
          name_, concurrency_, graph_, move(ft_), move(input_keys_), move(product_names)));
    }

  private:
    user_functions<T>& funcs_;
    std::string name_;
    std::size_t concurrency_{concurrency::serial};
    std::array<std::string, N> input_keys_;
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

    template <std::size_t I, typename U>
    auto
    get_handle_for(messages_t<N> const& messages)
    {
      using handle_arg_t = typename handle_for<U>::value_type;
      return std::get<I>(messages).store->template get_handle<handle_arg_t>(input_[I]);
    }

    template <std::size_t... Is>
    void
    call(std::function<void(generator&, Args...)> ft,
         generator& g,
         messages_t<N> const& messages,
         std::index_sequence<Is...>)
    {
      return std::invoke(ft, g, get_handle_for<Is, Args>(messages)...);
    }

  public:
    complete_splitter(std::string name,
                      std::size_t concurrency,
                      tbb::flow::graph& g,
                      std::function<void(generator&, Args...)>&& f,
                      std::array<std::string, N> input,
                      std::vector<std::string> provided_products) :
      declared_splitter{move(name), concurrency},
      input_{move(input)},
      provided_{move(provided_products)},
      multiplexer_{g},
      join_{g, type_for_t<MessageHasher, Args>{}...},
      splitter_{
        g,
        concurrency,
        [this, ft = std::move(f)](messages_t<N> const& messages) -> tbb::flow::continue_msg {
          auto const& msg = most_derived(messages);
          auto const& [store, message_id, _] = msg;
          if (store->is_flush()) {
            return {};
          }

          if (typename decltype(stores_)::const_accessor a; stores_.find(a, store->id())) {
            return {};
          }

          typename decltype(stores_)::accessor a;
          bool const new_insert = stores_.insert(a, store->id());
          if (!new_insert) {
            return {};
          }

          generator g{msg.store, multiplexer_, counter_};
          call(ft, g, messages, std::index_sequence_for<Args...>{});
          multiplexer_.try_put(g.flush_message());
          return {};
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

    std::span<std::string const, std::dynamic_extent>
    input() const override
    {
      return input_;
    }

    std::vector<std::string> const&
    provided_products() const override
    {
      return provided_;
    }

    void
    finalize(multiplexer::head_nodes_t head_nodes) override
    {
      multiplexer_.finalize(std::move(head_nodes));
    }

    std::array<std::string, N> input_;
    std::vector<std::string> provided_;
    multiplexer multiplexer_;
    join_or_none_t<N> join_;
    tbb::flow::function_node<messages_t<N>> splitter_;
    tbb::concurrent_hash_map<level_id, product_store_ptr> stores_;
    std::atomic<std::size_t> counter_{}; // Is this sufficient?  Probably not.
  };
}

#endif /* meld_core_declared_splitter_hpp */