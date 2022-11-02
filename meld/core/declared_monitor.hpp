#ifndef meld_core_declared_monitor_hpp
#define meld_core_declared_monitor_hpp

#include "meld/concurrency.hpp"
#include "meld/core/common_node_options.hpp"
#include "meld/core/concepts.hpp"
#include "meld/core/consumer.hpp"
#include "meld/core/detail/form_input_arguments.hpp"
#include "meld/core/detail/port_names.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/handle.hpp"
#include "meld/core/message.hpp"
#include "meld/core/product_store.hpp"
#include "meld/core/registrar.hpp"
#include "meld/graph/transition.hpp"
#include "meld/metaprogramming/type_deduction.hpp"
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

  template <is_monitor_like FT>
  class incomplete_monitor : public common_node_options<incomplete_monitor<FT>> {
    using common_node_options_t = common_node_options<incomplete_monitor<FT>>;
    using function_t = FT;
    using input_parameter_types = parameter_types<FT>;
    template <std::size_t Nactual, typename InputArgs>
    class complete_monitor;

  public:
    static constexpr auto N = number_parameters<FT>;

    incomplete_monitor(registrar<declared_monitors> reg,
                       std::string name,
                       tbb::flow::graph& g,
                       function_t f) :
      common_node_options_t{this},
      name_{move(name)},
      graph_{g},
      ft_{std::move(f)},
      reg_{std::move(reg)}
    {
    }

    template <typename... Args>
    incomplete_monitor& input(std::tuple<Args...> input_args)
    {
      static_assert(N == sizeof...(Args),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      auto processed_input_args =
        detail::form_input_arguments<input_parameter_types>(move(input_args));
      reg_.set([this, inputs = move(processed_input_args)] {
        auto product_names = detail::port_names(inputs);
        return std::make_unique<complete_monitor<size(product_names), decltype(inputs)>>(
          move(name_),
          common_node_options_t::concurrency(),
          common_node_options_t::release_preceding_filters(),
          graph_,
          move(ft_),
          move(inputs),
          move(product_names));
      });
      return *this;
    }

    using common_node_options_t::input;

  private:
    std::string name_;
    tbb::flow::graph& graph_;
    FT ft_;
    registrar<declared_monitors> reg_;
  };

  template <is_monitor_like FT>
  template <std::size_t Nactual, typename InputArgs>
  class incomplete_monitor<FT>::complete_monitor : public declared_monitor {
  public:
    complete_monitor(std::string name,
                     std::size_t concurrency,
                     std::vector<std::string> preceding_filters,
                     tbb::flow::graph& g,
                     function_t&& f,
                     InputArgs input,
                     std::array<std::string, Nactual> product_names) :
      declared_monitor{move(name), move(preceding_filters)},
      product_names_{move(product_names)},
      input_{move(input)},
      join_{make_join_or_none(g, std::make_index_sequence<Nactual>{})},
      monitor_{g,
               concurrency,
               [this, ft = std::move(f)](
                 messages_t<Nactual> const& messages) -> oneapi::tbb::flow::continue_msg {
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

                 call(ft, messages, std::make_index_sequence<N>{});
                 a->second = {};
                 return {};
               }}
    {
      make_edge(join_, monitor_);
    }

  private:
    tbb::flow::receiver<message>& port_for(std::string const& product_name) override
    {
      return receiver_for<Nactual>(join_, product_names_, product_name);
    }

    std::span<std::string const, std::dynamic_extent> input() const override
    {
      return product_names_;
    }

    template <std::size_t... Is>
    void call(function_t const& ft, messages_t<Nactual> const& messages, std::index_sequence<Is...>)
    {
      return std::invoke(ft, std::get<Is>(input_).retrieve(messages)...);
    }

    std::array<std::string, Nactual> product_names_;
    InputArgs input_;
    join_or_none_t<Nactual> join_;
    tbb::flow::function_node<messages_t<Nactual>> monitor_;
    tbb::concurrent_hash_map<level_id, product_store_ptr> stores_;
  };
}

#endif /* meld_core_declared_monitor_hpp */
