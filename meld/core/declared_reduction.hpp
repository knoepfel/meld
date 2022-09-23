#ifndef meld_core_declared_reduction_hpp
#define meld_core_declared_reduction_hpp

#include "meld/core/concurrency.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/handle.hpp"
#include "meld/core/message.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"
#include "meld/utilities/sized_tuple.hpp"

#include "oneapi/tbb/concurrent_unordered_map.h"
#include "oneapi/tbb/flow_graph.h"

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace meld {

  class declared_reduction {
  public:
    declared_reduction(std::string name, std::size_t concurrency);

    virtual ~declared_reduction();

    std::string const& name() const noexcept;
    std::size_t concurrency() const noexcept;
    tbb::flow::receiver<message>& port(std::string const& product_name);
    virtual tbb::flow::sender<message>& sender() = 0;
    virtual std::span<std::string const, std::dynamic_extent> input() const = 0;
    virtual std::span<std::string const, std::dynamic_extent> output() const = 0;

  private:
    virtual tbb::flow::receiver<message>& port_for(std::string const& product_name) = 0;

    std::string name_;
    std::size_t concurrency_;
  };

  using declared_reduction_ptr = std::unique_ptr<declared_reduction>;
  using declared_reductions = std::map<std::string, declared_reduction_ptr>;

  // Registering concrete reductions

  template <typename T, typename R, typename InitTuple, typename... Args>
  class incomplete_reduction {
    static constexpr auto N = sizeof...(Args);
    template <std::size_t M>
    class complete_reduction;
    class reduction_requires_output;

  public:
    incomplete_reduction(user_functions<T>& funcs,
                         std::string name,
                         tbb::flow::graph& g,
                         void (*f)(R&, Args...),
                         InitTuple initializer) :
      funcs_{funcs}, name_{move(name)}, graph_{g}, ft_{f}, initializer_{std::move(initializer)}
    {
    }

    template <typename FT>
    incomplete_reduction(user_functions<T>& funcs,
                         std::string name,
                         tbb::flow::graph& g,
                         FT f,
                         InitTuple initializer) :
      funcs_{funcs},
      name_{move(name)},
      graph_{g},
      ft_{std::move(f)},
      initializer_{std::move(initializer)}
    {
    }

    // Icky?
    incomplete_reduction& concurrency(std::size_t n)
    {
      concurrency_ = n;
      return *this;
    }

    auto input(std::array<std::string, N> input_keys);

    template <typename... Ts>
    auto input(Ts... ts)
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
    std::function<void(R&, Args...)> ft_;
    InitTuple initializer_;
  };

  template <typename T, typename R, typename InitTuple, typename... Args>
  template <std::size_t M>
  class incomplete_reduction<T, R, InitTuple, Args...>::complete_reduction :
    public declared_reduction {
    std::size_t port_index_for(std::string const& product_name)
    {
      auto it = std::find(cbegin(input_), cend(input_), product_name);
      if (it == cend(input_)) {
        throw std::runtime_error("Product name " + product_name + " not valid for transform.");
      }
      return std::distance(cbegin(input_), it);
    }

    template <std::size_t I>
    tbb::flow::receiver<message>& receiver_for(std::size_t const index)
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
    void call(std::function<void(R&, Args...)> const& ft,
              messages_t<N> const& messages,
              std::index_sequence<Is...>)
    {
      auto const& parent = most_derived(messages).store->parent();
      // FIXME: Not the safest approach!
      auto it = results_.find(parent->id());
      if (it == results_.end()) {
        it =
          results_
            .insert({parent->id(),
                     initialized_object(std::move(initializer_),
                                        std::make_index_sequence<std::tuple_size_v<InitTuple>>{})})
            .first;
      }
      return std::invoke(
        ft,
        *it->second,
        std::get<Is>(messages).store->template get_handle<typename handle_for<Args>::value_type>(
          input_[Is])...);
    }

    std::pair<bool, std::size_t> reduction_complete(product_store& parent_store)
    {
      auto it = entries_.find(parent_store.id());
      assert(it != cend(entries_));
      auto& entry = it->second;
      auto stop_number = entry->stop_after.load();
      if (entry->count.compare_exchange_strong(stop_number, -2u)) {
        commit_(parent_store);
        // FIXME: Would be good to free up memory here.
        return {true, entry->original_message_id};
      }
      return {};
    }

    void set_flush_value(level_id const& id, std::size_t const original_message_id)
    {

      auto it = entries_.find(id.parent());
      assert(it != cend(entries_));
      it->second->stop_after = id.back();
      it->second->original_message_id = original_message_id;
    }

  public:
    complete_reduction(std::string name,
                       std::size_t concurrency,
                       tbb::flow::graph& g,
                       std::function<void(R&, Args...)>&& f,
                       InitTuple initializer,
                       std::array<std::string, N> input,
                       std::array<std::string, M> output) :
      declared_reduction{move(name), concurrency},
      initializer_{std::move(initializer)},
      input_{move(input)},
      output_{move(output)},
      join_{g, type_for_t<MessageHasher, Args>{}...},
      reduction_{
        g, concurrency, [this, ft = std::move(f)](messages_t<N> const& messages, auto& outputs) {
          auto const& msg = most_derived(messages);
          auto const& [store, message_id, original_message_id] = msg;
          assert(store->parent());
          auto& parent_id = store->parent()->id();
          auto it = entries_.find(parent_id);
          if (it == cend(entries_)) {
            it = entries_.emplace(parent_id, std::make_unique<map_entry>()).first;
          }
          if (store->is_flush()) {
            set_flush_value(store->id(), original_message_id);
          }
          else {
            call(ft, messages, std::index_sequence_for<Args...>{});
            ++it->second->count;
          }
          auto const& parent = store->parent();
          if (auto const [complete, original_message_id] = reduction_complete(*parent); complete) {
            get<0>(outputs).try_put({parent, original_message_id});
          }
        }}
    {
      if constexpr (N > 1ull) {
        make_edge(join_, reduction_);
      }
      else {
        make_edge(join_.pass_through, reduction_);
      }
    }

  private:
    tbb::flow::receiver<message>& port_for(std::string const& product_name) override
    {
      if constexpr (N > 1ull) {
        auto const index = port_index_for(product_name);
        return receiver_for<0ull>(index);
      }
      else {
        return join_.pass_through;
      }
    }

    tbb::flow::sender<message>& sender() override { return output_port<0ull>(reduction_); }

    std::span<std::string const, std::dynamic_extent> input() const override { return input_; }
    std::span<std::string const, std::dynamic_extent> output() const override { return output_; }

    template <size_t... Is>
    std::unique_ptr<R> initialized_object(InitTuple&& tuple, std::index_sequence<Is...>) const
    {
      return std::unique_ptr<R>{
        new R{std::forward<std::tuple_element_t<Is, InitTuple>>(std::get<Is>(tuple))...}};
    }

    void commit_(product_store& store)
    {
      auto& result = results_.at(store.id());
      if constexpr (requires { result->send(); }) {
        store.add_product(output()[0], result->send());
      }
      else {
        store.add_product(output()[0], move(result));
      }
      // Reclaim some memory; it would be better to erase the entire
      // entry from the map, but that is not thread-safe.
      // N.B. Calling reset() is safe even if move(result) has been
      // called.
      result.reset();
    }

    InitTuple initializer_;
    std::array<std::string, N> input_;
    std::array<std::string, M> output_;
    join_or_none_t<N> join_;
    tbb::flow::multifunction_node<messages_t<N>, messages_t<1ull>> reduction_;
    tbb::concurrent_unordered_map<level_id, std::unique_ptr<R>> results_;
    struct map_entry {
      std::atomic<unsigned int> count{};
      std::atomic<unsigned int> stop_after{-1u};
      unsigned int original_message_id{}; // Necessary for matching inputs to downstream join nodes.
    };
    tbb::concurrent_unordered_map<level_id, std::unique_ptr<map_entry>> entries_;
  };

  template <typename T, typename R, typename InitTuple, typename... Args>
  auto incomplete_reduction<T, R, InitTuple, Args...>::input(std::array<std::string, N> input_keys)
  {
    if constexpr (std::same_as<
                    R,
                    void>) { // FIXME: It is suspect to have a void return type for reductions.
      funcs_.add_reduction(
        name_,
        std::make_unique<complete_reduction<0ull>>(
          name_, concurrency_, graph_, move(ft_), std::move(initializer_), move(input_keys), {}));
      return;
    }
    else {
      return reduction_requires_output{funcs_,
                                       move(name_),
                                       concurrency_,
                                       graph_,
                                       move(ft_),
                                       std::move(initializer_),
                                       move(input_keys)};
    }
  }

  template <typename T, typename R, typename InitTuple, typename... Args>
  class incomplete_reduction<T, R, InitTuple, Args...>::reduction_requires_output {
  public:
    reduction_requires_output(user_functions<T>& funcs,
                              std::string name,
                              std::size_t concurrency,
                              tbb::flow::graph& g,
                              std::function<void(R&, Args...)>&& f,
                              InitTuple initializer,
                              std::array<std::string, N> input_keys) :
      funcs_{funcs},
      name_{move(name)},
      concurrency_{concurrency},
      graph_{g},
      ft_{move(f)},
      initializer_{std::move(initializer)},
      input_keys_{move(input_keys)}
    {
    }

    template <std::size_t M>
    void output(std::array<std::string, M> output_keys)
    {
      funcs_.add_reduction(name_,
                           std::make_unique<complete_reduction<M>>(name_,
                                                                   concurrency_,
                                                                   graph_,
                                                                   move(ft_),
                                                                   std::move(initializer_),
                                                                   move(input_keys_),
                                                                   move(output_keys)));
    }

    template <typename... Ts>
    void output(Ts... ts)
    {
      static_assert(std::conjunction_v<std::is_convertible<Ts, std::string>...>);
      output(std::array<std::string, sizeof...(Ts)>{ts...});
    }

  private:
    user_functions<T>& funcs_;
    std::string name_;
    std::size_t concurrency_;
    tbb::flow::graph& graph_;
    std::function<void(R&, Args...)> ft_;
    InitTuple initializer_;
    std::array<std::string, N> input_keys_;
  };
}

#endif /* meld_core_declared_reduction_hpp */
