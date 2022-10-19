#ifndef meld_core_declared_reduction_hpp
#define meld_core_declared_reduction_hpp

#include "meld/concurrency.hpp"
#include "meld/core/consumer.hpp"
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
#include <concepts>
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

  class declared_reduction : public consumer {
  public:
    declared_reduction(std::string name, std::vector<std::string> preceding_filters);
    virtual ~declared_reduction();

    std::string const& name() const noexcept;
    virtual tbb::flow::sender<message>& sender() = 0;
    virtual tbb::flow::sender<message>& to_output() = 0;
    virtual std::span<std::string const, std::dynamic_extent> output() const = 0;

  private:
    std::string name_;
  };

  using declared_reduction_ptr = std::unique_ptr<declared_reduction>;
  using declared_reductions = std::map<std::string, declared_reduction_ptr>;

  // Registering concrete reductions

  template <typename T, typename R, typename InitTuple, typename... Args>
  class incomplete_reduction {
    static constexpr auto N = sizeof...(Args);
    using function_t = std::function<void(R&, Args...)>;
    template <std::size_t M>
    class complete_reduction;
    class reduction_requires_output;

  public:
    incomplete_reduction(component<T>& funcs,
                         std::string name,
                         tbb::flow::graph& g,
                         function_t f,
                         InitTuple initializer) :
      funcs_{funcs},
      name_{move(name)},
      graph_{g},
      ft_{move(f)},
      initializer_{std::move(initializer)}
    {
    }

    // Icky?
    incomplete_reduction& concurrency(std::size_t n)
    {
      concurrency_ = n;
      return *this;
    }

    incomplete_reduction& filtered_by(std::vector<std::string> preceding_filters)
    {
      preceding_filters_ = move(preceding_filters);
      return *this;
    }

    auto& filtered_by(std::convertible_to<std::string> auto&&... names)
    {
      return filtered_by(std::vector<std::string>{std::forward<decltype(names)>(names)...});
    }

    auto input(std::array<std::string, N> input_keys);

    auto input(std::convertible_to<std::string> auto&&... ts)
    {
      static_assert(N == sizeof...(ts),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return input(std::array<std::string, N>{std::forward<decltype(ts)>(ts)...});
    }

  private:
    component<T>& funcs_;
    std::string name_;
    std::size_t concurrency_{concurrency::serial};
    std::vector<std::string> preceding_filters_{};
    tbb::flow::graph& graph_;
    function_t ft_;
    InitTuple initializer_;
  };

  template <typename T, typename R, typename InitTuple, typename... Args>
  template <std::size_t M>
  class incomplete_reduction<T, R, InitTuple, Args...>::complete_reduction :
    public declared_reduction {
  public:
    complete_reduction(std::string name,
                       std::size_t concurrency,
                       std::vector<std::string> preceding_filters,
                       tbb::flow::graph& g,
                       function_t&& f,
                       InitTuple initializer,
                       std::array<std::string, N> input,
                       std::array<std::string, M> output) :
      declared_reduction{move(name), move(preceding_filters)},
      initializer_{std::move(initializer)},
      input_{move(input)},
      output_{move(output)},
      join_{g, type_for_t<MessageHasher, Args>{}...},
      reduction_{
        g, concurrency, [this, ft = std::move(f)](messages_t<N> const& messages, auto& outputs) {
          // N.B. The assumption is that a reduction will *never* need to cache the
          //      product store it creates.
          auto const& msg = most_derived(messages);
          auto const& [store, original_message_id] = std::tie(msg.store, msg.original_id);
          auto const parent_id = store->id().parent();
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
          auto parent = make_product_store(parent_id, this->name());
          if (auto const [complete, original_message_id] = reduction_complete(*parent); complete) {
            get<0>(outputs).try_put({parent, original_message_id});
          }
        }}
    {
      make_edge(join_, reduction_);
    }

  private:
    tbb::flow::receiver<message>& port_for(std::string const& product_name) override
    {
      return receiver_for<N>(join_, input_, product_name);
    }

    tbb::flow::sender<message>& sender() override { return output_port<0ull>(reduction_); }
    tbb::flow::sender<message>& to_output() override { return sender(); }
    std::span<std::string const, std::dynamic_extent> input() const override { return input_; }
    std::span<std::string const, std::dynamic_extent> output() const override { return output_; }

    template <std::size_t... Is>
    void call(function_t const& ft, messages_t<N> const& messages, std::index_sequence<Is...>)
    {
      auto const parent_id = most_derived(messages).store->id().parent();
      // FIXME: Not the safest approach!
      auto it = results_.find(parent_id);
      if (it == results_.end()) {
        it =
          results_
            .insert({parent_id,
                     initialized_object(std::move(initializer_),
                                        std::make_index_sequence<std::tuple_size_v<InitTuple>>{})})
            .first;
      }
      return std::invoke(ft, *it->second, get_handle_for<Is, N, Args>(messages, input_)...);
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
      // Reclaim some memory; it would be better to erase the entire entry from the map,
      // but that is not thread-safe.

      // N.B. Calling reset() is safe even if move(result) has been called.
      result.reset();
    }

    InitTuple initializer_;
    std::array<std::string, N> input_;
    std::array<std::string, M> output_;
    join_or_none_t<N> join_;
    tbb::flow::multifunction_node<messages_t<N>, messages_t<1>> reduction_;
    tbb::concurrent_unordered_map<level_id, std::unique_ptr<R>> results_;
    struct map_entry {
      std::atomic<unsigned int> count{};
      std::atomic<unsigned int> stop_after{-1u};
      unsigned int original_message_id{}; // Necessary for matching inputs to downstream join nodes.
    };
    tbb::concurrent_unordered_map<level_id, std::unique_ptr<map_entry>> entries_;
  };

  // =================================================================================
  // Implementation

  template <typename T, typename R, typename InitTuple, typename... Args>
  auto incomplete_reduction<T, R, InitTuple, Args...>::input(std::array<std::string, N> input_keys)
  {
    if constexpr (std::same_as<
                    R,
                    void>) { // FIXME: It is suspect to have a void return type for reductions.
      funcs_.add_reduction(name_,
                           std::make_unique<complete_reduction<0ull>>(name_,
                                                                      concurrency_,
                                                                      move(preceding_filters_),
                                                                      graph_,
                                                                      move(ft_),
                                                                      std::move(initializer_),
                                                                      move(input_keys),
                                                                      {}));
      return;
    }
    else {
      return reduction_requires_output{funcs_,
                                       move(name_),
                                       concurrency_,
                                       move(preceding_filters_),
                                       graph_,
                                       move(ft_),
                                       std::move(initializer_),
                                       move(input_keys)};
    }
  }

  template <typename T, typename R, typename InitTuple, typename... Args>
  class incomplete_reduction<T, R, InitTuple, Args...>::reduction_requires_output {
  public:
    reduction_requires_output(component<T>& funcs,
                              std::string name,
                              std::size_t concurrency,
                              std::vector<std::string> preceding_filters,
                              tbb::flow::graph& g,
                              function_t&& f,
                              InitTuple initializer,
                              std::array<std::string, N> input_keys) :
      funcs_{funcs},
      name_{move(name)},
      preceding_filters_{move(preceding_filters)},
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
                                                                   move(preceding_filters_),
                                                                   graph_,
                                                                   move(ft_),
                                                                   std::move(initializer_),
                                                                   move(input_keys_),
                                                                   move(output_keys)));
    }

    // FIXME: Does it make sense for a reduction to have more than one output?
    void output(std::convertible_to<std::string> auto&&... ts)
    {
      output(std::array<std::string, sizeof...(ts)>{std::forward<decltype(ts)>(ts)...});
    }

  private:
    component<T>& funcs_;
    std::string name_;
    std::vector<std::string> preceding_filters_;
    std::size_t concurrency_;
    tbb::flow::graph& graph_;
    function_t ft_;
    InitTuple initializer_;
    std::array<std::string, N> input_keys_;
  };
}

#endif /* meld_core_declared_reduction_hpp */
