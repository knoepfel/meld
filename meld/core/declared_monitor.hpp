#ifndef meld_core_declared_monitor_hpp
#define meld_core_declared_monitor_hpp

#include "meld/core/concepts.hpp"
#include "meld/core/detail/port_names.hpp"
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
#include <type_traits>
#include <utility>

namespace meld {

  class declared_monitor : public products_consumer {
  public:
    declared_monitor(std::string name,
                     std::vector<std::string> preceding_filters);
    virtual ~declared_monitor();
  };

  using declared_monitor_ptr = std::unique_ptr<declared_monitor>;
  using declared_monitors = std::map<std::string, declared_monitor_ptr>;

  // Registering concrete monitors

  template <is_monitor_like FT, typename InputArgs>
  class complete_monitor : public declared_monitor, public detect_flush_flag {

    static constexpr auto N = std::tuple_size_v<InputArgs>;
    using function_t = FT;
    using stores_t = tbb::concurrent_hash_map<level_id::hash_type, bool>;
    using accessor = stores_t::accessor;

  public:
    complete_monitor(std::string name,
                     std::size_t concurrency,
                     std::vector<std::string> preceding_filters,
                     tbb::flow::graph& g,
                     function_t&& f,
                     InputArgs input,
                     std::array<std::string, N> product_names) :
      declared_monitor{std::move(name), std::move(preceding_filters)},
      product_names_{std::move(product_names)},
      input_{std::move(input)},
      join_{make_join_or_none(g, std::make_index_sequence<N>{})},
      monitor_{g,
               concurrency,
               [this, ft = std::move(f)](
                 messages_t<N> const& messages) -> oneapi::tbb::flow::continue_msg {
                 auto const& msg = most_derived(messages);
                 auto const& [store, message_id] = std::tie(msg.store, msg.id);
                 // meld::stage const st{store->is_flush() ? meld::stage::flush : meld::stage::process};
                 // spdlog::debug("Monitor {} received {} ({})",
                 //               this->name(),
                 //               store->id(),
                 //               to_string(st));
                 if (store->is_flush()) {
                   flag_accessor ca;
                   flag_for(store->id()->hash(), ca).flush_received(message_id);
                 }
                 else if (accessor a; needs_new(store, a)) {
                   call(ft, messages, std::make_index_sequence<N>{});
                   a->second = true;
                   flag_accessor ca;
                   flag_for(store->id()->hash(), ca).mark_as_processed();
                 }
                 auto const id_hash = store->id()->hash();
                 if (const_flag_accessor ca; flag_for(id_hash, ca) && ca->second->is_flush()) {
                   erase_flag(ca);
                   stores_.erase(id_hash);
                 }
                 return {};
               }}
    {
      make_edge(join_, monitor_);
    }

    ~complete_monitor()
    {
      if (stores_.size() > 0ull) {
        spdlog::warn("Monitor {} has {} cached stores.", name(), stores_.size());
      }
      for (auto const& [id, _] : stores_) {
        spdlog::debug(" => ID: {}", id);
      }
    }

  private:
    tbb::flow::receiver<message>& port_for(std::string const& product_name) override
    {
      return receiver_for<N>(join_, product_names_, product_name);
    }

    std::vector<tbb::flow::receiver<message>*> ports() override { return input_ports<N>(join_); }

    std::span<std::string const, std::dynamic_extent> input() const override
    {
      return product_names_;
    }

    bool needs_new(product_store_const_ptr const& store, accessor& a)
    {
      if (stores_.count(store->id()->hash()) > 0ull) {
        return false;
      }

      bool const new_insert = stores_.insert(a, store->id()->hash());
      if (!new_insert) {
        return false;
      }
      return true;
    }

    template <std::size_t... Is>
    void call(function_t const& ft, messages_t<N> const& messages, std::index_sequence<Is...>)
    {
      ++calls_;
      return std::invoke(ft, std::get<Is>(input_).retrieve(messages)...);
    }

    std::size_t num_calls() const final { return calls_.load(); }

    std::array<std::string, N> product_names_;
    InputArgs input_;
    join_or_none_t<N> join_;
    tbb::flow::function_node<messages_t<N>> monitor_;
    tbb::concurrent_hash_map<level_id::hash_type, bool> stores_;
    std::atomic<std::size_t> calls_;
  };
}

#endif /* meld_core_declared_monitor_hpp */
