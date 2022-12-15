#ifndef meld_core_declared_filter_hpp
#define meld_core_declared_filter_hpp

#include "meld/concurrency.hpp"
#include "meld/core/common_node_options.hpp"
#include "meld/core/detail/form_input_arguments.hpp"
#include "meld/core/detail/port_names.hpp"
#include "meld/core/filter/filter_impl.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/handle.hpp"
#include "meld/core/message.hpp"
#include "meld/core/product_store.hpp"
#include "meld/core/products_consumer.hpp"
#include "meld/core/registrar.hpp"
#include "meld/core/store_counters.hpp"
#include "meld/graph/transition.hpp"
#include "meld/metaprogramming/type_deduction.hpp"
#include "meld/utilities/sized_tuple.hpp"

#include "oneapi/tbb/concurrent_unordered_map.h"
#include "oneapi/tbb/flow_graph.h"
#include "spdlog/spdlog.h"

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
#include <tuple>
#include <type_traits>
#include <utility>

namespace meld {

  class declared_filter : public products_consumer {
  public:
    declared_filter(std::string name, std::vector<std::string> preceding_filters);
    virtual ~declared_filter();

    virtual tbb::flow::sender<filter_result>& sender() = 0;
  };

  using declared_filter_ptr = std::unique_ptr<declared_filter>;
  using declared_filters = std::map<std::string, declared_filter_ptr>;

  // Registering concrete filters

  template <typename FT>
  class incomplete_filter : public common_node_options<incomplete_filter<FT>> {
    using common_node_options_t = common_node_options<incomplete_filter<FT>>;
    using function_t = FT;
    using input_parameter_types = parameter_types<FT>;
    template <std::size_t Nactual, typename InputArgs>
    class complete_filter;

  public:
    static constexpr auto N = number_parameters<FT>;

    incomplete_filter(registrar<declared_filters> reg,
                      std::string name,
                      tbb::flow::graph& g,
                      function_t f) :
      common_node_options_t{this}, name_{move(name)}, graph_{g}, ft_{move(f)}, reg_{std::move(reg)}
    {
    }

    template <typename... Args>
    incomplete_filter& input(std::tuple<Args...> input_args)
    {
      static_assert(N == sizeof...(Args),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      auto processed_input_args =
        detail::form_input_arguments<input_parameter_types>(move(input_args));
      reg_.set([this, inputs = move(processed_input_args)] {
        auto product_names = detail::port_names(inputs);
        return std::make_unique<complete_filter<size(product_names), decltype(inputs)>>(
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
    function_t ft_;
    registrar<declared_filters> reg_;
  };

  template <typename FT>
  template <std::size_t Nactual, typename InputArgs>
  class incomplete_filter<FT>::complete_filter : public declared_filter, public detect_flush_flag {
    using results_t = tbb::concurrent_hash_map<level_id::hash_type, filter_result>;
    using accessor = results_t::accessor;
    using const_accessor = results_t::const_accessor;

  public:
    complete_filter(std::string name,
                    std::size_t concurrency,
                    std::vector<std::string> preceding_filters,
                    tbb::flow::graph& g,
                    function_t&& f,
                    InputArgs input,
                    std::array<std::string, Nactual> product_names) :
      declared_filter{move(name), move(preceding_filters)},
      product_names_{move(product_names)},
      input_{move(input)},
      join_{make_join_or_none(g, std::make_index_sequence<Nactual>{})},
      filter_{g,
              concurrency,
              [this, ft = std::move(f)](messages_t<Nactual> const& messages) -> filter_result {
                auto const& msg = most_derived(messages);
                auto const& [store, message_id] = std::tie(msg.store, msg.id);
                filter_result result{};
                if (store->is_flush()) {
                  flag_accessor ca;
                  flag_for(store->id().parent().hash(), ca).flush_received(message_id);
                }
                else if (const_accessor a; results_.find(a, store->id().hash())) {
                  result = {message_id, a->second.result};
                }
                else if (accessor a; results_.insert(a, store->id().hash())) {
                  bool const rc = call(ft, messages, std::make_index_sequence<N>{});
                  result = a->second = {message_id, rc};
                  flag_accessor ca;
                  flag_for(store->id().hash(), ca).mark_as_processed();
                }

                auto const id_hash =
                  store->is_flush() ? store->id().parent().hash() : store->id().hash();
                if (const_flag_accessor ca; flag_for(id_hash, ca) && ca->second->is_flush()) {
                  results_.erase(id_hash);
                  erase_flag(ca);
                }
                return result;
              }}
    {
      make_edge(join_, filter_);
    }

    ~complete_filter()
    {
      if (results_.size() > 0ull) {
        spdlog::warn("Filter {} has {} cached results.", name(), results_.size());
      }
    }

  private:
    tbb::flow::receiver<message>& port_for(std::string const& product_name) override
    {
      return receiver_for<N>(join_, product_names_, product_name);
    }

    tbb::flow::sender<filter_result>& sender() override { return filter_; }
    std::span<std::string const, std::dynamic_extent> input() const override
    {
      return product_names_;
    }

    template <std::size_t... Is>
    bool call(function_t const& ft, messages_t<Nactual> const& messages, std::index_sequence<Is...>)
    {
      return std::invoke(ft, std::get<Is>(input_).retrieve(messages)...);
    }

    std::array<std::string, Nactual> product_names_;
    InputArgs input_;
    join_or_none_t<Nactual> join_;
    tbb::flow::function_node<messages_t<Nactual>, filter_result> filter_;
    results_t results_;
  };

}

#endif /* meld_core_declared_filter_hpp */
