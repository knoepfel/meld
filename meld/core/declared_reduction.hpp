#ifndef meld_core_declared_reduction_hpp
#define meld_core_declared_reduction_hpp

#include "meld/concurrency.hpp"
#include "meld/core/concepts.hpp"
#include "meld/core/detail/port_names.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"
#include "meld/core/node_options.hpp"
#include "meld/core/products_consumer.hpp"
#include "meld/core/reduction/send.hpp"
#include "meld/core/registrar.hpp"
#include "meld/core/store_counters.hpp"
#include "meld/model/handle.hpp"
#include "meld/model/level_id.hpp"
#include "meld/model/product_store.hpp"

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

  template <is_reduction_like FT, typename InputArgs>
  class pre_reduction {
    using input_parameter_types =
      skip_first_type<function_parameter_types<FT>>; // Skip reduction object
    static constexpr auto N = std::tuple_size_v<input_parameter_types>;
    using R = std::decay_t<std::tuple_element_t<0, function_parameter_types<FT>>>;

    static constexpr std::size_t M = 1; // hard-coded for now
    using function_t = FT;

    template <typename InitTuple>
    class total_reduction;

  public:
    pre_reduction(registrar<declared_reductions> reg,
                  std::string name,
                  std::size_t concurrency,
                  std::vector<std::string> preceding_filters,
                  tbb::flow::graph& g,
                  function_t&& f,
                  InputArgs input_args) :
      name_{std::move(name)},
      concurrency_{concurrency},
      preceding_filters_{std::move(preceding_filters)},
      graph_{g},
      ft_{std::move(f)},
      input_args_{std::move(input_args)},
      product_names_{detail::port_names(input_args_)},
      reg_{std::move(reg)}
    {
    }

    template <std::size_t Msize>
    auto& to(std::array<std::string, Msize> output_keys)
    {
      static_assert(
        M == Msize,
        "The number of function parameters is not the same as the number of returned output "
        "objects.");
      output_keys_ = std::move(output_keys);
      reg_.set([this] { return create(std::make_tuple()); });
      return *this;
    }

    auto& to(std::convertible_to<std::string> auto&&... ts)
    {
      static_assert(
        M == sizeof...(ts),
        "The number of function parameters is not the same as the number of returned output "
        "objects.");
      return to(std::array<std::string, M>{std::forward<decltype(ts)>(ts)...});
    }

    auto& for_each(std::string const& level_name)
    {
      reduction_interval_ = level_name;
      return *this;
    }

    auto& initialized_with(auto&&... ts)
    {
      reg_.set([this, init = std::tuple{ts...}] { return create(std::move(init)); });
      return *this;
    }

  private:
    template <typename T>
    declared_reduction_ptr create(T init)
    {
      if (empty(reduction_interval_)) {
        throw std::runtime_error(
          "The reduction range must be specified using the 'over(...)' syntax.");
      }
      return std::make_unique<total_reduction<decltype(init)>>(std::move(name_),
                                                               concurrency_,
                                                               std::move(preceding_filters_),
                                                               graph_,
                                                               std::move(ft_),
                                                               std::move(init),
                                                               std::move(input_args_),
                                                               std::move(product_names_),
                                                               std::move(output_keys_),
                                                               std::move(reduction_interval_));
    }

    std::string name_;
    std::size_t concurrency_;
    std::vector<std::string> preceding_filters_;
    tbb::flow::graph& graph_;
    function_t ft_;
    InputArgs input_args_;
    std::array<specified_label, N> product_names_;
    std::string reduction_interval_{level_id::base().level_name()};
    std::array<std::string, M> output_keys_;
    registrar<declared_reductions> reg_;
  };

  template <is_reduction_like FT, typename InputArgs>
  template <typename InitTuple>
  class pre_reduction<FT, InputArgs>::total_reduction :
    public declared_reduction,
    private count_stores {

  public:
    total_reduction(std::string name,
                    std::size_t concurrency,
                    std::vector<std::string> preceding_filters,
                    tbb::flow::graph& g,
                    function_t&& f,
                    InitTuple initializer,
                    InputArgs input,
                    std::array<specified_label, N> product_names,
                    std::array<std::string, M> output,
                    std::string reduction_interval) :
      declared_reduction{std::move(name), std::move(preceding_filters)},
      initializer_{std::move(initializer)},
      product_names_{std::move(product_names)},
      input_{std::move(input)},
      output_{std::move(output)},
      reduction_interval_{std::move(reduction_interval)},
      join_{make_join_or_none(g, std::make_index_sequence<N>{})},
      reduction_{
        g, concurrency, [this, ft = std::move(f)](messages_t<N> const& messages, auto& outputs) {
          // N.B. The assumption is that a reduction will *never* need to cache
          //      the product store it creates.  Any flush messages *do not* need
          //      to be propagated to downstream nodes.
          auto const& msg = most_derived(messages);
          auto const& [store, original_message_id] = std::tie(msg.store, msg.original_id);

          if (not store->is_flush() and not store->id()->parent(reduction_interval_)) {
            return;
          }

          if (store->is_flush()) {
            // Downstream nodes always get the flush.
            get<0>(outputs).try_put(msg);
            if (store->id()->level_name() != reduction_interval_) {
              return;
            }
          }

          auto const& reduction_store =
            store->is_flush() ? store : store->parent(reduction_interval_);
          assert(reduction_store);
          auto const& id_for_counter = reduction_store->id();

          counter_accessor ca;
          auto& counter = counter_for(id_for_counter->hash(), ca);
          if (store->is_flush()) {
            counter.set_flush_value(store, original_message_id);
          }
          else {
            call(ft, messages, std::make_index_sequence<N>{});
            counter.increment(store->id()->level_name());
          }
          ca.release();

          const_counter_accessor cca;
          bool const has_counter = counter_for(id_for_counter->hash(), cca);

          if (has_counter && cca->second->is_flush()) {
            auto parent = reduction_store->make_continuation(this->name());
            commit_(*parent);
            // FIXME: This msg.eom value may be wrong!
            get<0>(outputs).try_put({parent, msg.eom, counter.original_message_id()});
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

    std::vector<tbb::flow::receiver<message>*> ports() override { return input_ports<N>(join_); }

    tbb::flow::sender<message>& sender() override { return output_port<0ull>(reduction_); }
    tbb::flow::sender<message>& to_output() override { return sender(); }
    std::span<specified_label const, std::dynamic_extent> input() const override
    {
      return product_names_;
    }
    std::span<std::string const, std::dynamic_extent> output() const override { return output_; }

    template <std::size_t... Is>
    void call(function_t const& ft, messages_t<N> const& messages, std::index_sequence<Is...>)
    {
      auto const& parent_id = *most_derived(messages).store->id()->parent(reduction_interval_);
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
      ++calls_;
      return std::invoke(ft, *it->second, std::get<Is>(input_).retrieve(messages)...);
    }

    std::size_t num_calls() const final { return calls_.load(); }

    template <size_t... Is>
    auto initialized_object(InitTuple&& tuple, std::index_sequence<Is...>) const
    {
      return std::unique_ptr<R>{
        new R{std::forward<std::tuple_element_t<Is, InitTuple>>(std::get<Is>(tuple))...}};
    }

    void commit_(product_store& store)
    {
      auto& result = results_.at(*store.id());
      if constexpr (requires { send(*result); }) {
        store.add_product(output()[0], send(*result));
      }
      else {
        store.add_product(output()[0], std::move(*result));
      }
      // Reclaim some memory; it would be better to erase the entire entry from the map,
      // but that is not thread-safe.
      result.reset();
    }

    InitTuple initializer_;
    std::array<specified_label, N> product_names_;
    InputArgs input_;
    std::array<std::string, M> output_;
    std::string reduction_interval_;
    join_or_none_t<N> join_;
    tbb::flow::multifunction_node<messages_t<N>, messages_t<1>> reduction_;
    tbb::concurrent_unordered_map<level_id, std::unique_ptr<R>> results_;
    std::atomic<std::size_t> calls_;
  };
}

#endif /* meld_core_declared_reduction_hpp */
