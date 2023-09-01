#ifndef meld_core_declared_filter_hpp
#define meld_core_declared_filter_hpp

#include "meld/concurrency.hpp"
#include "meld/core/detail/port_names.hpp"
#include "meld/core/filter/filter_impl.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"
#include "meld/core/products_consumer.hpp"
#include "meld/core/registrar.hpp"
#include "meld/core/store_counters.hpp"
#include "meld/metaprogramming/type_deduction.hpp"
#include "meld/model/handle.hpp"
#include "meld/model/level_id.hpp"
#include "meld/model/product_store.hpp"
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
    declared_filter(std::string name,
                    std::vector<std::string> preceding_filters);
    virtual ~declared_filter();

    virtual tbb::flow::sender<filter_result>& sender() = 0;
  };

  using declared_filter_ptr = std::unique_ptr<declared_filter>;
  using declared_filters = std::map<std::string, declared_filter_ptr>;

  // Registering concrete filters

  template <typename FT, typename InputArgs>
  class complete_filter : public declared_filter, public detect_flush_flag {
    static constexpr auto N = std::tuple_size_v<InputArgs>;
    using function_t = FT;
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
                    std::array<std::string, N> product_names) :
      declared_filter{std::move(name), std::move(preceding_filters)},
      product_names_{std::move(product_names)},
      input_{std::move(input)},
      join_{make_join_or_none(g, std::make_index_sequence<N>{})},
      filter_{
        g, concurrency, [this, ft = std::move(f)](messages_t<N> const& messages) -> filter_result {
          auto const& msg = most_derived(messages);
          auto const& [store, message_id] = std::tie(msg.store, msg.id);
          filter_result result{};
          if (store->is_flush()) {
            flag_accessor ca;
            flag_for(store->id()->hash(), ca).flush_received(message_id);
          }
          else if (const_accessor a; results_.find(a, store->id()->hash())) {
            result = {msg.eom, message_id, a->second.result};
          }
          else if (accessor a; results_.insert(a, store->id()->hash())) {
            bool const rc = call(ft, messages, std::make_index_sequence<N>{});
            result = a->second = {msg.eom, message_id, rc};
            flag_accessor ca;
            flag_for(store->id()->hash(), ca).mark_as_processed();
          }

          auto const id_hash = store->id()->hash();
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

    std::vector<tbb::flow::receiver<message>*> ports() override { return input_ports<N>(join_); }

    tbb::flow::sender<filter_result>& sender() override { return filter_; }
    std::span<std::string const, std::dynamic_extent> input() const override
    {
      return product_names_;
    }

    template <std::size_t... Is>
    bool call(function_t const& ft, messages_t<N> const& messages, std::index_sequence<Is...>)
    {
      ++calls_;
      return std::invoke(ft, std::get<Is>(input_).retrieve(messages)...);
    }

    std::size_t num_calls() const final { return calls_.load(); }

    std::array<std::string, N> product_names_;
    InputArgs input_;
    join_or_none_t<N> join_;
    tbb::flow::function_node<messages_t<N>, filter_result> filter_;
    results_t results_;
    std::atomic<std::size_t> calls_;
  };

}

#endif /* meld_core_declared_filter_hpp */
