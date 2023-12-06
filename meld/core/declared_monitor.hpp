#ifndef meld_core_declared_monitor_hpp
#define meld_core_declared_monitor_hpp

#include "meld/core/concepts.hpp"
#include "meld/core/detail/port_names.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"
#include "meld/core/products_consumer.hpp"
#include "meld/core/registrar.hpp"
#include "meld/core/specified_label.hpp"
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
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace meld {

  class declared_monitor : public products_consumer {
  public:
    declared_monitor(std::string name, std::vector<std::string> preceding_filters);
    virtual ~declared_monitor();
  };

  using declared_monitor_ptr = std::unique_ptr<declared_monitor>;
  using declared_monitors = std::map<std::string, declared_monitor_ptr>;

  // Registering concrete monitors
  template <is_monitor_like FT, typename InputArgs>
  class pre_monitor {
    static constexpr std::size_t N = std::tuple_size_v<InputArgs>;
    using function_t = FT;

    class complete_monitor;

  public:
    pre_monitor(registrar<declared_monitors> reg,
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
      product_labels_{detail::port_names(input_args_)},
      reg_{std::move(reg)}
    {
      reg_.set([this] { return create(); });
    }

    auto& for_each(std::string const& domain)
    {
      for (auto& allowed_domain : product_labels_ | std::views::transform(to_domain)) {
        if (empty(allowed_domain)) {
          allowed_domain = domain;
        }
      }
      return *this;
    }

  private:
    declared_monitor_ptr create()
    {
      return std::make_unique<complete_monitor>(std::move(name_),
                                                concurrency_,
                                                std::move(preceding_filters_),
                                                graph_,
                                                std::move(ft_),
                                                std::move(input_args_),
                                                std::move(product_labels_));
    }
    std::string name_;
    std::size_t concurrency_;
    std::vector<std::string> preceding_filters_;
    tbb::flow::graph& graph_;
    function_t ft_;
    InputArgs input_args_;
    std::array<specified_label, N> product_labels_;
    registrar<declared_monitors> reg_;
  };

  template <is_monitor_like FT, typename InputArgs>
  class pre_monitor<FT, InputArgs>::complete_monitor :
    public declared_monitor,
    private detect_flush_flag {

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
                     std::array<specified_label, N> product_labels) :
      declared_monitor{std::move(name), std::move(preceding_filters)},
      product_labels_{std::move(product_labels)},
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
    tbb::flow::receiver<message>& port_for(specified_label const& product_label) override
    {
      return receiver_for<N>(join_, product_labels_, product_label);
    }

    std::vector<tbb::flow::receiver<message>*> ports() override { return input_ports<N>(join_); }

    std::span<specified_label const, std::dynamic_extent> input() const override
    {
      return product_labels_;
    }

    bool needs_new(product_store_const_ptr const& store, accessor& a)
    {
      if (stores_.count(store->id()->hash()) > 0ull) {
        return false;
      }
      return stores_.insert(a, store->id()->hash());
    }

    template <std::size_t... Is>
    void call(function_t const& ft, messages_t<N> const& messages, std::index_sequence<Is...>)
    {
      ++calls_;
      return std::invoke(ft, std::get<Is>(input_).retrieve(messages)...);
    }

    std::size_t num_calls() const final { return calls_.load(); }

    std::array<specified_label, N> product_labels_;
    InputArgs input_;
    join_or_none_t<N> join_;
    tbb::flow::function_node<messages_t<N>> monitor_;
    tbb::concurrent_hash_map<level_id::hash_type, bool> stores_;
    std::atomic<std::size_t> calls_;
  };
}

#endif /* meld_core_declared_monitor_hpp */
