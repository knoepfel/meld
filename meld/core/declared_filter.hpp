#ifndef meld_core_declared_filter_hpp
#define meld_core_declared_filter_hpp

#include "meld/concurrency.hpp"
#include "meld/core/consumer.hpp"
#include "meld/core/filter/filter_impl.hpp"
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

  class declared_filter : public consumer {
  public:
    declared_filter(std::string name, std::vector<std::string> preceding_filters);
    virtual ~declared_filter();

    std::string const& name() const noexcept;
    virtual tbb::flow::sender<filter_result>& sender() = 0;

  private:
    std::string name_;
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

    // Icky?
    incomplete_filter& filtered_by(std::vector<std::string> preceding_filters)
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
      funcs_.add_filter(
        name_,
        std::make_unique<complete_filter>(
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
  class incomplete_filter<T, Args...>::complete_filter : public declared_filter {
  public:
    complete_filter(std::string name,
                    std::size_t concurrency,
                    std::vector<std::string> preceding_filters,
                    tbb::flow::graph& g,
                    function_t&& f,
                    std::array<std::string, N> input) :
      declared_filter{move(name), move(preceding_filters)},
      input_{move(input)},
      join_{g, type_for_t<MessageHasher, Args>{}...},
      filter_{
        g, concurrency, [this, ft = std::move(f)](messages_t<N> const& messages) -> filter_result {
          auto const& msg = most_derived(messages);
          auto const& [store, message_id] = std::tie(msg.store, msg.id);
          if (store->is_flush()) {
            // FIXME: Depending on timing, the following may introduce weird effects
            //        (e.g. deleting cached stores before the user function has been
            //        invoked).
            results_.erase(store->id().parent());
            return {};
          }

          if (typename decltype(results_)::const_accessor a; results_.find(a, store->id())) {
            return {message_id, a->second.result};
          }

          typename decltype(results_)::accessor a;
          if (results_.insert(a, store->id())) {
            bool const rc = call(ft, messages, std::index_sequence_for<Args...>{});
            a->second = {message_id, rc};
          }
          return a->second;
        }}
    {
      make_edge(join_, filter_);
    }

  private:
    tbb::flow::receiver<message>& port_for(std::string const& product_name) override
    {
      return receiver_for<N>(join_, input_, product_name);
    }

    tbb::flow::sender<filter_result>& sender() override { return filter_; }
    std::span<std::string const, std::dynamic_extent> input() const override { return input_; }

    template <std::size_t... Is>
    bool call(function_t const& ft, messages_t<N> const& messages, std::index_sequence<Is...>)
    {
      return std::invoke(ft, get_handle_for<Is, N, Args>(messages, input_)...);
    }

    std::array<std::string, N> input_;
    join_or_none_t<N> join_;
    tbb::flow::function_node<messages_t<N>, filter_result> filter_;
    tbb::concurrent_hash_map<level_id, filter_result> results_;
  };
}

#endif /* meld_core_declared_filter_hpp */
