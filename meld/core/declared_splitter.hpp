#ifndef meld_core_declared_splitter_hpp
#define meld_core_declared_splitter_hpp

#include "meld/concurrency.hpp"
#include "meld/core/common_node_options.hpp"
#include "meld/core/concepts.hpp"
#include "meld/core/detail/form_input_arguments.hpp"
#include "meld/core/detail/port_names.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"
#include "meld/core/multiplexer.hpp"
#include "meld/core/products_consumer.hpp"
#include "meld/core/registrar.hpp"
#include "meld/core/store_counters.hpp"
#include "meld/model/handle.hpp"
#include "meld/model/level_id.hpp"
#include "meld/model/product_store.hpp"
#include "meld/utilities/sized_tuple.hpp"

#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/flow_graph.h"
#include "spdlog/spdlog.h"

#include <array>
#include <atomic>
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
#include <vector>

namespace meld {

  class generator {
  public:
    explicit generator(product_store_const_ptr const& parent,
                       std::string const& node_name,
                       multiplexer& m,
                       tbb::flow::broadcast_node<message>& to_output,
                       std::atomic<std::size_t>& counter);
    void make_child(std::size_t i, std::string const& new_level_name, products new_products = {});
    message flush_message();

  private:
    product_store_ptr parent_;
    std::string const& node_name_;
    multiplexer& multiplexer_;
    tbb::flow::broadcast_node<message>& to_output_;
    std::atomic<std::size_t>& counter_;
    std::atomic<std::size_t> calls_{};
    std::size_t const original_message_id_{counter_};
  };

  class declared_splitter : public products_consumer {
  public:
    declared_splitter(std::string name,
                      std::vector<std::string> preceding_filters,
                      std::vector<std::string> receive_stores);
    virtual ~declared_splitter();

    virtual tbb::flow::sender<message>& to_output() = 0;
    virtual std::vector<std::string> const& provided_products() const = 0;
    virtual void finalize(multiplexer::head_ports_t head_ports) = 0;
  };

  using declared_splitter_ptr = std::unique_ptr<declared_splitter>;
  using declared_splitters = std::map<std::string, declared_splitter_ptr>;

  // =====================================================================================

  template <is_splitter_like FT>
  class incomplete_splitter : public common_node_options<incomplete_splitter<FT>> {
    using common_node_options_t = common_node_options<incomplete_splitter<FT>>;
    using function_t = FT;
    using input_parameter_types = skip_first_type<parameter_types<FT>>; // Skip generator object
    template <std::size_t Nactual, typename InputArgs>
    class splitter_requires_provides;
    template <std::size_t Nactual, typename InputArgs>
    class complete_splitter;

  public:
    static constexpr auto N = std::tuple_size_v<input_parameter_types>;

    incomplete_splitter(registrar<declared_splitters> reg,
                        configuration const* config,
                        std::string name,
                        tbb::flow::graph& g,
                        function_t f) :
      common_node_options_t{config},
      name_{move(name)},
      graph_{g},
      ft_{move(f)},
      reg_{std::move(reg)}
    {
    }

    template <typename... Args>
    auto input(std::tuple<Args...> input_args)
    {
      static_assert(N == sizeof...(Args),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      auto processed_input_args =
        detail::form_input_arguments<input_parameter_types>(move(input_args));
      auto product_names = detail::port_names(processed_input_args);
      return splitter_requires_provides<size(product_names), decltype(processed_input_args)>{
        std::move(reg_),
        move(name_),
        common_node_options_t::concurrency(),
        common_node_options_t::release_preceding_filters(),
        common_node_options_t::release_store_names(),
        graph_,
        move(ft_),
        move(processed_input_args),
        move(product_names)};
    }

    using common_node_options_t::input;

  private:
    std::string name_;
    tbb::flow::graph& graph_;
    function_t ft_;
    registrar<declared_splitters> reg_;
  };

  // =====================================================================================

  template <is_splitter_like FT>
  template <std::size_t Nactual, typename InputArgs>
  class incomplete_splitter<FT>::splitter_requires_provides {
  public:
    splitter_requires_provides(registrar<declared_splitters> reg,
                               std::string name,
                               std::size_t concurrency,
                               std::vector<std::string> preceding_filters,
                               std::vector<std::string> receive_stores,
                               tbb::flow::graph& g,
                               function_t&& f,
                               InputArgs input_args,
                               std::array<std::string, Nactual> product_names) :
      name_{move(name)},
      concurrency_{concurrency},
      preceding_filters_{move(preceding_filters)},
      receive_stores_{move(receive_stores)},
      graph_{g},
      ft_{move(f)},
      input_args_{move(input_args)},
      product_names_{move(product_names)},
      reg_{std::move(reg)}
    {
    }

    auto& provides(std::vector<std::string> output_product_names)
    {
      reg_.set([this, outputs = move(output_product_names)] {
        return std::make_unique<complete_splitter<Nactual, InputArgs>>(move(name_),
                                                                       concurrency_,
                                                                       move(preceding_filters_),
                                                                       move(receive_stores_),
                                                                       graph_,
                                                                       move(ft_),
                                                                       move(input_args_),
                                                                       move(product_names_),
                                                                       move(outputs));
      });
      return *this;
    }

  private:
    std::string name_;
    std::size_t concurrency_;
    std::vector<std::string> preceding_filters_;
    std::vector<std::string> receive_stores_;
    tbb::flow::graph& graph_;
    function_t ft_;
    InputArgs input_args_;
    std::array<std::string, Nactual> product_names_;
    registrar<declared_splitters> reg_;
  };

  // =====================================================================================

  template <is_splitter_like FT>
  template <std::size_t Nactual, typename InputArgs>
  class incomplete_splitter<FT>::complete_splitter :
    public declared_splitter,
    public detect_flush_flag {
    using stores_t = tbb::concurrent_hash_map<level_id::hash_type, product_store_ptr>;
    using accessor = stores_t::accessor;
    using const_accessor = stores_t::const_accessor;

  public:
    complete_splitter(std::string name,
                      std::size_t concurrency,
                      std::vector<std::string> preceding_filters,
                      std::vector<std::string> receive_stores,
                      tbb::flow::graph& g,
                      function_t&& f,
                      InputArgs input,
                      std::array<std::string, Nactual> product_names,
                      std::vector<std::string> provided_products) :
      declared_splitter{move(name), move(preceding_filters), move(receive_stores)},
      input_{move(input)},
      product_names_{move(product_names)},
      provided_{move(provided_products)},
      multiplexer_{g},
      join_{make_join_or_none(g, std::make_index_sequence<Nactual>{})},
      splitter_{
        g,
        concurrency,
        [this, ft = std::move(f)](messages_t<Nactual> const& messages) -> tbb::flow::continue_msg {
          auto const& msg = most_derived(messages);
          auto const& store = msg.store;
          if (store->is_flush()) {
            flag_accessor ca;
            flag_for(store->id()->parent()->hash(), ca).flush_received(msg.id);
          }
          else if (accessor a; stores_.insert(a, store->id()->hash())) {
            generator g{msg.store, this->name(), multiplexer_, to_output_, counter_};
            call(ft, g, messages, std::make_index_sequence<N>{});
            multiplexer_.try_put(g.flush_message());

            flag_accessor ca;
            flag_for(store->id()->hash(), ca).mark_as_processed();
          }

          auto const id_hash =
            store->is_flush() ? store->id()->parent()->hash() : store->id()->hash();
          if (const_flag_accessor ca; flag_for(id_hash, ca) && ca->second->is_flush()) {
            stores_.erase(id_hash);
            erase_flag(ca);
          }
          return {};
        }},
      to_output_{g}
    {
      make_edge(join_, splitter_);
    }

    ~complete_splitter()
    {
      if (stores_.size() > 0ull) {
        spdlog::warn("Splitter {} has {} cached stores.", name(), stores_.size());
      }
      for (auto const& [_, store] : stores_) {
        spdlog::debug(" => ID: ", store->id()->to_string());
      }
    }

  private:
    tbb::flow::receiver<message>& port_for(std::string const& product_name) override
    {
      return receiver_for<Nactual>(join_, product_names_, product_name);
    }
    std::vector<tbb::flow::receiver<message>*> ports() override
    {
      return input_ports<Nactual>(join_);
    }

    tbb::flow::sender<message>& to_output() override { return to_output_; }

    std::span<std::string const, std::dynamic_extent> input() const override
    {
      return product_names_;
    }
    std::vector<std::string> const& provided_products() const override { return provided_; }

    void finalize(multiplexer::head_ports_t head_ports) override
    {
      multiplexer_.finalize(std::move(head_ports));
    }

    template <std::size_t... Is>
    void call(function_t const& ft,
              generator& g,
              messages_t<Nactual> const& messages,
              std::index_sequence<Is...>)
    {
      return std::invoke(ft, g, std::get<Is>(input_).retrieve(messages)...);
    }

    InputArgs input_;
    std::array<std::string, Nactual> product_names_;
    std::vector<std::string> provided_;
    multiplexer multiplexer_;
    join_or_none_t<Nactual> join_;
    tbb::flow::function_node<messages_t<Nactual>> splitter_;
    tbb::flow::broadcast_node<message> to_output_;
    tbb::concurrent_hash_map<level_id::hash_type, product_store_ptr> stores_;
    std::atomic<std::size_t> counter_{}; // Is this sufficient?  Probably not.
  };
}

#endif /* meld_core_declared_splitter_hpp */
