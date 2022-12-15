#ifndef meld_core_declared_reduction_hpp
#define meld_core_declared_reduction_hpp

#include "meld/concurrency.hpp"
#include "meld/core/common_node_options.hpp"
#include "meld/core/concepts.hpp"
#include "meld/core/detail/form_input_arguments.hpp"
#include "meld/core/detail/port_names.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/handle.hpp"
#include "meld/core/message.hpp"
#include "meld/core/product_store.hpp"
#include "meld/core/products_consumer.hpp"
#include "meld/core/registrar.hpp"
#include "meld/core/store_counters.hpp"
#include "meld/graph/transition.hpp"

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

  class declared_reduction : public products_consumer {
  public:
    declared_reduction(std::string name, std::vector<std::string> preceding_filters);
    virtual ~declared_reduction();

    virtual tbb::flow::sender<message>& sender() = 0;
    virtual tbb::flow::sender<message>& to_output() = 0;
    virtual std::span<std::string const, std::dynamic_extent> output() const = 0;
  };

  using declared_reduction_ptr = std::unique_ptr<declared_reduction>;
  using declared_reductions = std::map<std::string, declared_reduction_ptr>;

  // Registering concrete reductions

  template <is_reduction_like FT, typename InitTuple>
  class incomplete_reduction : public common_node_options<incomplete_reduction<FT, InitTuple>> {
    using common_node_options_t = common_node_options<incomplete_reduction<FT, InitTuple>>;
    using function_t = FT;
    using input_parameter_types = skip_first_type<parameter_types<FT>>; // Skip reduction object
    template <std::size_t Nactual, typename InputArgs>
    class reduction_requires_output;
    template <std::size_t Nactual, std::size_t M, typename InputArgs>
    class complete_reduction;

  public:
    static constexpr auto N = std::tuple_size_v<input_parameter_types>;

    incomplete_reduction(registrar<declared_reductions> reg,
                         boost::json::object const* config,
                         std::string name,
                         tbb::flow::graph& g,
                         function_t f,
                         InitTuple initializer) :
      common_node_options_t{this, config},
      reg_{std::move(reg)},
      name_{move(name)},
      graph_{g},
      ft_{move(f)},
      initializer_{std::move(initializer)}
    {
    }

    template <typename... InputArgs>
    auto input(std::tuple<InputArgs...> input_args)
    {
      static_assert(N == sizeof...(InputArgs),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      auto processed_input_args =
        detail::form_input_arguments<input_parameter_types>(move(input_args));
      auto product_names = detail::port_names(processed_input_args);
      return reduction_requires_output<size(product_names), decltype(processed_input_args)>{
        std::move(reg_),
        move(name_),
        common_node_options_t::concurrency(),
        common_node_options_t::release_preceding_filters(),
        graph_,
        move(ft_),
        std::move(initializer_),
        move(processed_input_args),
        move(product_names)};
    }

    using common_node_options_t::input;

  private:
    registrar<declared_reductions> reg_;
    std::string name_;
    tbb::flow::graph& graph_;
    function_t ft_;
    InitTuple initializer_;
  };

  // =====================================================================================

  template <is_reduction_like FT, typename InitTuple>
  template <std::size_t Nactual, typename InputArgs>
  class incomplete_reduction<FT, InitTuple>::reduction_requires_output {
    static constexpr std::size_t M = 1ull; // TODO: Hard-wired, for now

  public:
    reduction_requires_output(registrar<declared_reductions> reg,
                              std::string name,
                              std::size_t concurrency,
                              std::vector<std::string> preceding_filters,
                              tbb::flow::graph& g,
                              function_t&& f,
                              InitTuple initializer,
                              InputArgs input_args,
                              std::array<std::string, Nactual> product_names) :
      name_{move(name)},
      preceding_filters_{move(preceding_filters)},
      concurrency_{concurrency},
      graph_{g},
      ft_{move(f)},
      initializer_{std::move(initializer)},
      input_args_{move(input_args)},
      product_names_{move(product_names)},
      reg_{std::move(reg)}
    {
    }

    template <std::size_t Msize>
    void output(std::array<std::string, Msize> output_keys)
    {
      static_assert(
        M == Msize,
        "The number of function parameters is not the same as the number of returned output "
        "objects.");
      reg_.set([this, outputs = move(output_keys)] {
        return std::make_unique<complete_reduction<Nactual, M, InputArgs>>(move(name_),
                                                                           concurrency_,
                                                                           move(preceding_filters_),
                                                                           graph_,
                                                                           move(ft_),
                                                                           std::move(initializer_),
                                                                           move(input_args_),
                                                                           move(product_names_),
                                                                           move(outputs));
      });
    }

    void output(std::convertible_to<std::string> auto&&... ts)
    {
      static_assert(
        M == sizeof...(ts),
        "The number of function parameters is not the same as the number of returned output "
        "objects.");
      output(std::array<std::string, sizeof...(ts)>{std::forward<decltype(ts)>(ts)...});
    }

  private:
    std::string name_;
    std::vector<std::string> preceding_filters_;
    std::size_t concurrency_;
    tbb::flow::graph& graph_;
    function_t ft_;
    InitTuple initializer_;
    InputArgs input_args_;
    std::array<std::string, Nactual> product_names_;
    registrar<declared_reductions> reg_;
  };

  // =================================================================================

  template <is_reduction_like FT, typename InitTuple>
  template <std::size_t Nactual, std::size_t M, typename InputArgs>
  class incomplete_reduction<FT, InitTuple>::complete_reduction :
    public declared_reduction,
    public count_stores {
    using R = std::decay_t<std::tuple_element_t<0, parameter_types<FT>>>;

  public:
    complete_reduction(std::string name,
                       std::size_t concurrency,
                       std::vector<std::string> preceding_filters,
                       tbb::flow::graph& g,
                       function_t&& f,
                       InitTuple initializer,
                       InputArgs input,
                       std::array<std::string, Nactual> product_names,
                       std::array<std::string, M> output) :
      declared_reduction{move(name), move(preceding_filters)},
      initializer_{std::move(initializer)},
      product_names_{move(product_names)},
      input_{move(input)},
      output_{move(output)},
      join_{make_join_or_none(g, std::make_index_sequence<Nactual>{})},
      reduction_{g,
                 concurrency,
                 [this, ft = std::move(f)](messages_t<Nactual> const& messages, auto& outputs) {
                   // N.B. The assumption is that a reduction will *never* need to cache
                   //      the product store it creates.  Any flush messages *do not* need
                   //      to be propagated to downstream nodes.
                   auto const& msg = most_derived(messages);
                   auto const& [store, original_message_id] = std::tie(msg.store, msg.original_id);
                   auto const parent_id = store->id().parent();

                   // meld::stage const st{store->is_flush() ? meld::stage::flush : meld::stage::process};
                   // spdlog::debug("Reduction {} received {} from {} ({}, original message {})",
                   //               this->name(),
                   //               store->id(),
                   //               store->source(),
                   //               to_string(st),
                   //               original_message_id);
                   counter_accessor ca;
                   auto& counter = counter_for(parent_id.hash(), ca);
                   if (depth_ != -1ull and parent_id.depth() != depth_ - 1) {
                     return;
                   }
                   if (store->is_flush()) {
                     counter.set_flush_value(store->id(), original_message_id);
                   }
                   else {
                     // FIXME: Check re. depth?
                     depth_ = store->id().depth();
                     call(ft, messages, std::make_index_sequence<N>{});
                     counter.increment();
                   }
                   ca.release();

                   if (parent_id.depth() != depth_ - 1)
                     return;

                   const_counter_accessor cca;
                   // spdlog::trace(" => Counter for {} ", parent_id);
                   bool const has_counter = counter_for(parent_id.hash(), cca);
                   if (has_counter && cca->second->is_flush(/*&parent_id*/)) {
                     // spdlog::trace(" -> Flushing for {}", parent_id);
                     auto parent = make_product_store(parent_id, this->name());
                     commit_(*parent);
                     // spdlog::trace(" -> Sending message ID {}", counter.original_message_id());
                     get<0>(outputs).try_put({parent, counter.original_message_id()});
                     erase_counter(cca);
                   }
                 }}
    {
      make_edge(join_, reduction_);
    }

  private:
    tbb::flow::receiver<message>& port_for(std::string const& product_name) override
    {
      return receiver_for<N>(join_, product_names_, product_name);
    }

    std::vector<tbb::flow::receiver<message>*> ports() override
    {
      return input_ports<Nactual>(join_);
    }

    tbb::flow::sender<message>& sender() override { return output_port<0ull>(reduction_); }
    tbb::flow::sender<message>& to_output() override { return sender(); }
    std::span<std::string const, std::dynamic_extent> input() const override
    {
      return product_names_;
    }
    std::span<std::string const, std::dynamic_extent> output() const override { return output_; }

    template <std::size_t... Is>
    void call(function_t const& ft, messages_t<Nactual> const& messages, std::index_sequence<Is...>)
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
      return std::invoke(ft, *it->second, std::get<Is>(input_).retrieve(messages)...);
    }

    template <size_t... Is>
    auto initialized_object(InitTuple&& tuple, std::index_sequence<Is...>) const
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
    std::array<std::string, Nactual> product_names_;
    InputArgs input_;
    std::array<std::string, M> output_;
    join_or_none_t<Nactual> join_;
    tbb::flow::multifunction_node<messages_t<Nactual>, messages_t<1>> reduction_;
    tbb::concurrent_unordered_map<level_id, std::unique_ptr<R>> results_;
    std::atomic<std::size_t> depth_{-1ull};
  };
}

#endif /* meld_core_declared_reduction_hpp */
