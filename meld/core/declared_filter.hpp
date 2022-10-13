#ifndef meld_core_declared_filter_hpp
#define meld_core_declared_filter_hpp

#include "meld/core/concurrency.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/handle.hpp"
#include "meld/core/message.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"
#include "meld/utilities/sized_tuple.hpp"
#include "meld/utilities/type_deduction.hpp"

#include "oneapi/tbb/concurrent_unordered_map.h"
#include "oneapi/tbb/flow_graph.h"

#include <array>
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

  class declared_filter {
  public:
    declared_filter(std::string name, std::size_t concurrency);

    virtual ~declared_filter();

    std::string const& name() const noexcept;
    std::size_t concurrency() const noexcept;
    tbb::flow::receiver<message>& port(std::string const& product_name);
    virtual tbb::flow::sender<message>& sender() = 0;
    virtual tbb::flow::sender<message>& to_output() = 0;
    virtual std::span<std::string const, std::dynamic_extent> input() const = 0;

  private:
    virtual tbb::flow::receiver<message>& port_for(std::string const& product_name) = 0;

    std::string name_;
    std::size_t concurrency_;
  };

  using declared_filter_ptr = std::unique_ptr<declared_filter>;
  using declared_filters = std::map<std::string, declared_filter_ptr>;

  // Registering concrete filters

  template <typename T, typename... Args>
  class incomplete_filter {
    static constexpr auto N = sizeof...(Args);
    using function_t = std::function<bool(Args...)>;
    class complete_filter;

  public:
    incomplete_filter(component<T>& funcs, std::string name, tbb::flow::graph& g, function_t f) :
      funcs_{funcs}, name_{move(name)}, graph_{g}, ft_{move(f)}
    {
    }

    // Icky?
    incomplete_filter& concurrency(std::size_t n)
    {
      concurrency_ = n;
      return *this;
    }

    template <std::size_t Nsize>
    auto input(std::array<std::string, Nsize> input_keys)
    {
      static_assert(N == Nsize,
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      funcs_.add_filter(name_,
                        std::make_unique<complete_filter>(
                          name_, concurrency_, graph_, move(ft_), move(input_keys)));
    }

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
    component<T>& funcs_;
    std::string name_;
    std::size_t concurrency_{concurrency::serial};
    tbb::flow::graph& graph_;
    function_t ft_;
  };

  template <typename T, typename... Args>
  class incomplete_filter<T, Args...>::complete_filter : public declared_filter {
  public:
    complete_filter(std::string name,
                    std::size_t concurrency,
                    tbb::flow::graph& g,
                    function_t&& f,
                    std::array<std::string, N> input) :
      declared_filter{move(name), concurrency},
      input_{move(input)},
      join_{g, type_for_t<MessageHasher, Args>{}...},
      filter_{
        g, concurrency, [this, ft = std::move(f)](messages_t<N> const& messages, auto& output) {
          auto const& msg = most_derived(messages);
          auto const& [store, message_id] = std::tie(msg.store, msg.id);
          auto& [stay_in_graph, to_output] = output;
          if (store->is_flush()) {
            // FIXME: Depending on timing, the following may introduce
            //        weird effects (e.g. deleting cached stores
            //        before the user function has been invoked).
            stores_.erase(store->id().parent());
            stay_in_graph.try_put(msg);
            return;
          }

          if (typename decltype(stores_)::const_accessor a; stores_.find(a, store->id())) {
            stay_in_graph.try_put({a->second, message_id});
            return;
          }

          typename decltype(stores_)::accessor a;
          bool const new_insert = stores_.insert(a, store->id());
          if (!new_insert) {
            stay_in_graph.try_put({a->second, message_id});
            return;
          }

          bool const rc [[maybe_unused]] = call(ft, messages, std::index_sequence_for<Args...>{});
          a->second = {};
        }}
    {
      make_edge(join_, filter_);
    }

  private:
    tbb::flow::receiver<message>& port_for(std::string const& product_name) override
    {
      return receiver_for<N>(join_, input_, product_name);
    }

    tbb::flow::sender<message>& sender() override { return output_port<0>(filter_); }
    tbb::flow::sender<message>& to_output() override { return output_port<1>(filter_); }
    std::span<std::string const, std::dynamic_extent> input() const override { return input_; }

    template <std::size_t... Is>
    bool call(function_t const& ft, messages_t<N> const& messages, std::index_sequence<Is...>)
    {
      return std::invoke(ft, get_handle_for<Is, N, Args>(messages, input_)...);
    }

    std::array<std::string, N> input_;
    join_or_none_t<N> join_;
    tbb::flow::multifunction_node<messages_t<N>, messages_t<2u>> filter_;
    tbb::concurrent_hash_map<level_id, product_store_ptr> stores_;
  };
}

#endif /* meld_core_declared_filter_hpp */
