#ifndef meld_core_declared_transform_hpp
#define meld_core_declared_transform_hpp

#include "meld/concurrency.hpp"
#include "meld/core/concepts.hpp"
#include "meld/core/consumer.hpp"
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

  class declared_transform : public consumer {
  public:
    declared_transform(std::string name, std::vector<std::string> preceding_filters);
    virtual ~declared_transform();

    std::string const& name() const noexcept;

    virtual tbb::flow::sender<message>& sender() = 0;
    virtual tbb::flow::sender<message>& to_output() = 0;
    virtual std::span<std::string const, std::dynamic_extent> output() const = 0;

  private:
    std::string name_;
  };

  using declared_transform_ptr = std::unique_ptr<declared_transform>;
  using declared_transforms = std::map<std::string, declared_transform_ptr>;

  // Registering concrete transforms

  template <typename T, not_void R, typename... Args>
  class incomplete_transform {
    static constexpr auto N = sizeof...(Args);
    using function_t = std::function<R(Args...)>;
    template <std::size_t M>
    class complete_transform;
    class transform_requires_output;

  public:
    incomplete_transform(component<T>& funcs, std::string name, tbb::flow::graph& g, function_t f) :
      funcs_{funcs}, name_{move(name)}, graph_{g}, ft_{move(f)}
    {
    }

    // Icky?
    incomplete_transform& concurrency(std::size_t n)
    {
      concurrency_ = n;
      return *this;
    }

    // Icky?
    incomplete_transform& filtered_by(std::vector<std::string> preceding_filters)
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
      return transform_requires_output{funcs_,
                                       move(name_),
                                       concurrency_,
                                       move(preceding_filters_),
                                       graph_,
                                       move(ft_),
                                       move(input_keys)};
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

  template <typename T, not_void R, typename... Args>
  class incomplete_transform<T, R, Args...>::transform_requires_output {
    static constexpr std::size_t M = number_types<R>;

  public:
    transform_requires_output(component<T>& funcs,
                              std::string name,
                              std::size_t concurrency,
                              std::vector<std::string> preceding_filters,
                              tbb::flow::graph& g,
                              function_t&& f,
                              std::array<std::string, N> input_keys) :
      funcs_{funcs},
      name_{move(name)},
      concurrency_{concurrency},
      preceding_filters_{move(preceding_filters)},
      graph_{g},
      ft_{move(f)},
      input_keys_{move(input_keys)}
    {
    }

    template <std::size_t Msize>
    void output(std::array<std::string, Msize> output_keys)
    {
      static_assert(
        M == Msize,
        "The number of function parameters is not the same as the number of returned output "
        "objects.");
      funcs_.add_transform(name_,
                           std::make_unique<complete_transform<M>>(name_,
                                                                   concurrency_,
                                                                   move(preceding_filters_),
                                                                   graph_,
                                                                   move(ft_),
                                                                   move(input_keys_),
                                                                   move(output_keys)));
    }

    void output(std::convertible_to<std::string> auto&&... ts)
    {
      static_assert(
        M == sizeof...(ts),
        "The number of function parameters is not the same as the number of returned output "
        "objects.");
      output(std::array<std::string, M>{std::forward<decltype(ts)>(ts)...});
    }

  private:
    component<T>& funcs_;
    std::string name_;
    std::size_t concurrency_;
    std::vector<std::string> preceding_filters_;
    tbb::flow::graph& graph_;
    function_t ft_;
    std::array<std::string, N> input_keys_;
  };

  template <typename T, not_void R, typename... Args>
  template <std::size_t M>
  class incomplete_transform<T, R, Args...>::complete_transform : public declared_transform {
  public:
    complete_transform(std::string name,
                       std::size_t concurrency,
                       std::vector<std::string> preceding_filters,
                       tbb::flow::graph& g,
                       function_t&& f,
                       std::array<std::string, N> input,
                       std::array<std::string, M> output) :
      declared_transform{move(name), move(preceding_filters)},
      input_{move(input)},
      output_{move(output)},
      join_{g, type_for_t<MessageHasher, Args>{}...},
      transform_{
        g, concurrency, [this, ft = std::move(f)](messages_t<N> const& messages, auto& output) {
          auto const& msg = most_derived(messages);
          auto const& [store, message_id] = std::tie(msg.store, msg.id);
          auto& [stay_in_graph, to_output] = output;
          if (store->is_flush()) {
            // FIXME: Depending on timing, the following may introduce weird effects
            //        (e.g. deleting cached stores before the user function has been
            //        invoked).
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

          auto result = call(ft, messages, std::index_sequence_for<Args...>{});
          auto new_store = make_product_store(store->id(), this->name());
          add_to(*new_store, result, output_);
          a->second = new_store;

          message const new_msg{a->second, message_id};
          stay_in_graph.try_put(new_msg);
          to_output.try_put(new_msg);
        }}
    {
      make_edge(join_, transform_);
    }

  private:
    tbb::flow::receiver<message>& port_for(std::string const& product_name) override
    {
      return receiver_for<N>(join_, input_, product_name);
    }

    tbb::flow::sender<message>& sender() override { return output_port<0>(transform_); }
    tbb::flow::sender<message>& to_output() override { return output_port<1>(transform_); }
    std::span<std::string const, std::dynamic_extent> input() const override { return input_; }
    std::span<std::string const, std::dynamic_extent> output() const override { return output_; }

    template <std::size_t... Is>
    R call(function_t const& ft, messages_t<N> const& messages, std::index_sequence<Is...>)
    {
      return std::invoke(ft, get_handle_for<Is, N, Args>(messages, input_)...);
    }

    std::array<std::string, N> input_;
    std::array<std::string, M> output_;
    join_or_none_t<N> join_;
    tbb::flow::multifunction_node<messages_t<N>, messages_t<2u>> transform_;
    tbb::concurrent_hash_map<level_id, product_store_ptr> stores_;
  };
}

#endif /* meld_core_declared_transform_hpp */
