#include "meld/core/framework_graph.hpp"

#include "meld/core/edge_maker.hpp"
#include "meld/core/product_store.hpp"

#include "spdlog/cfg/env.h"

namespace meld {
  framework_graph::framework_graph(run_once_t, product_store_ptr store) :
    framework_graph{[store, executed = false]() mutable -> product_store_ptr {
      if (executed) {
        return nullptr;
      }
      executed = true;
      return store;
    }}
  {
  }

  framework_graph::framework_graph(std::function<product_store_ptr()> f) :
    src_{graph_,
         [this, user_function = move(f)](tbb::flow_control& fc) mutable -> message {
           auto store = user_function();
           if (not store) {
             fc.stop();
             return {};
           }

           ++calls_;
           if (store->is_flush()) {
             // Original message ID no longer needed after this message.
             auto h = original_message_ids_.extract(store->id().parent());
             assert(h);
             std::size_t const original_message_id = h.mapped();
             return {store, calls_, original_message_id};
           }

           // debug("Inserting message ID for ", store->id(), ": ", calls_);
           original_message_ids_.try_emplace(store->id(), calls_);
           return {store, calls_};
         }},
    multiplexer_{graph_}
  {
    spdlog::cfg::load_env_levels();
  }

  void framework_graph::execute(std::string const& dot_file_name)
  {
    finalize(dot_file_name);
    run();
  }

  void framework_graph::run()
  {
    src_.activate();
    graph_.wait_for_all();
  }

  namespace {
    template <typename T>
    auto internal_edges_for_filters(oneapi::tbb::flow::graph& g,
                                    declared_filters& filters,
                                    T const& consumers)
    {
      std::map<std::string, result_collector> result;
      for (auto const& [name, consumer] : consumers) {
        auto const& preceding_filters = consumer->filtered_by();
        if (empty(preceding_filters)) {
          continue;
        }

        auto [it, success] = result.try_emplace(name, g, *consumer);
        // debug("Preceding filters for ", name, ": ", preceding_filters);
        for (auto const& filter_name : preceding_filters) {
          auto fit = filters.find(filter_name);
          if (fit == cend(filters)) {
            throw std::runtime_error("A non-existent filter with the name '" + filter_name +
                                     "' was specified for " + name);
          }
          make_edge(fit->second->sender(), it->second.filter_port());
        }
      }
      return result;
    }
  }

  void framework_graph::finalize(std::string const& dot_file_name)
  {
    filter_collectors_.merge(internal_edges_for_filters(graph_, filters_, filters_));
    filter_collectors_.merge(internal_edges_for_filters(graph_, filters_, outputs_));
    filter_collectors_.merge(internal_edges_for_filters(graph_, filters_, reductions_));
    filter_collectors_.merge(internal_edges_for_filters(graph_, filters_, splitters_));
    filter_collectors_.merge(internal_edges_for_filters(graph_, filters_, transforms_));

    edge_maker make_edges{dot_file_name, outputs_, transforms_, reductions_};
    make_edges(src_,
               multiplexer_,
               filter_collectors_,
               consumers{transforms_, {.shape = "ellipse"}},
               consumers{reductions_, {.arrowtail = "dot", .shape = "ellipse"}},
               consumers{filters_, {.shape = "invtrapezium"}},
               consumers{splitters_, {.shape = "trapezium"}});
  }
}
