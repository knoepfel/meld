#ifndef meld_core_declared_monitor_hpp
#define meld_core_declared_monitor_hpp

#include "meld/concurrency.hpp"
#include "meld/core/concepts.hpp"
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
#include <concepts>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace meld {

  class declared_monitor : public consumer {
  public:
    declared_monitor(std::string name, std::vector<std::string> preceding_filters);
    virtual ~declared_monitor();

    std::string const& name() const noexcept;

  private:
    std::string name_;
  };

  using declared_monitor_ptr = std::unique_ptr<declared_monitor>;
  using declared_monitors = std::map<std::string, declared_monitor_ptr>;

  // Registering concrete monitors

  template <typename T, typename... Args>
  class incomplete_monitor {
    static constexpr auto N = sizeof...(Args);
    using function_t = std::function<void(Args...)>;
    class complete_monitor;

  public:
    incomplete_monitor(component<T>& funcs, std::string name, tbb::flow::graph& g, function_t f) :
      funcs_{funcs}, name_{move(name)}, graph_{g}, ft_{move(f)}
    {
    }

    // Icky?
    incomplete_monitor& concurrency(std::size_t n)
    {
      concurrency_ = n;
      return *this;
    }

    // Icky?
    incomplete_monitor& filtered_by(std::vector<std::string> preceding_filters)
    {
      preceding_filters_ = move(preceding_filters);
      return *this;
    }

    auto& filtered_by(std::convertible_to<std::string> auto&&... names)
    {
      return filtered_by(std::vector<std::string>{std::forward<decltype(names)>(names)...});
    }

    template <std::size_t Nsize>
    auto input(std::array<std::string, Nsize> input_keys)
    {
      static_assert(N == Nsize,
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      funcs_.add_monitor(
        name_,
        std::make_unique<complete_monitor>(
          name_, concurrency_, move(preceding_filters_), graph_, move(ft_), move(input_keys)));
    }

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
  };

  template <typename T, typename... Args>
  class incomplete_monitor<T, Args...>::complete_monitor : public declared_monitor {
  public:
    complete_monitor(std::string name,
                     std::size_t concurrency,
                     std::vector<std::string> preceding_filters,
                     tbb::flow::graph& g,
                     function_t&& f,
                     std::array<std::string, N> input) :
      declared_monitor{move(name), move(preceding_filters)},
      input_{move(input)},
      join_{g, type_for_t<MessageHasher, Args>{}...},
      monitor_{g,
               concurrency,
               [this, ft = std::move(f)](
                 messages_t<N> const& messages) -> oneapi::tbb::flow::continue_msg {
                 auto const& msg = most_derived(messages);
                 auto const& [store, message_id] = std::tie(msg.store, msg.id);
                 if (store->is_flush()) {
                   // FIXME: Depending on timing, the following may introduce weird effects
                   //        (e.g. deleting cached stores before the user function has been
                   //        invoked).
                   stores_.erase(store->id().parent());
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

                 call(ft, messages, std::index_sequence_for<Args...>{});
                 a->second = {};
                 return {};
               }}
    {
      make_edge(join_, monitor_);
    }

  private:
    tbb::flow::receiver<message>& port_for(std::string const& product_name) override
    {
      return receiver_for<N>(join_, input_, product_name);
    }

    std::span<std::string const, std::dynamic_extent> input() const override { return input_; }

    template <std::size_t... Is>
    void call(function_t const& ft, messages_t<N> const& messages, std::index_sequence<Is...>)
    {
      return std::invoke(ft, get_handle_for<Is, N, Args>(messages, input_)...);
    }

    std::array<std::string, N> input_;
    join_or_none_t<N> join_;
    tbb::flow::function_node<messages_t<N>> monitor_;
    tbb::concurrent_hash_map<level_id, product_store_ptr> stores_;
  };
}

#endif /* meld_core_declared_monitor_hpp */
