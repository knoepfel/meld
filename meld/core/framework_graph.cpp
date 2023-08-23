#include "meld/core/framework_graph.hpp"

#include "meld/concurrency.hpp"
#include "meld/core/edge_maker.hpp"
#include "meld/model/level_counter.hpp"
#include "meld/model/product_store.hpp"

#include "spdlog/cfg/env.h"

#include <cassert>

namespace meld {
  level_sentry::level_sentry(flush_counters& counters,
                             message_sender& sender,
                             product_store_ptr store) :
    counters_{counters}, sender_{sender}, store_{store}, depth_{store_->id()->depth()}
  {
    counters_.update(store_->id());
  }

  level_sentry::~level_sentry()
  {
    auto flush_result = counters_.extract(store_->id());
    auto flush_store = store_->make_flush();
    if (not flush_result.empty()) {
      flush_store->add_product("[flush]",
                               std::make_shared<flush_counts const>(std::move(flush_result)));
    }
    sender_.send_flush(std::move(flush_store));
  }

  std::size_t level_sentry::depth() const noexcept { return depth_; }

  framework_graph::framework_graph(product_store_ptr store, int const max_parallelism) :
    framework_graph{[store]() mutable {
                      // Returns non-null store, then replaces it with nullptr, thus
                      // resulting in one graph execution.
                      return std::exchange(store, nullptr);
                    },
                    max_parallelism}
  {
  }

  // FIXME: The algorithm below should support user-specified flush stores.
  framework_graph::framework_graph(std::function<product_store_ptr()> next_store,
                                   int const max_parallelism) :
    parallelism_limit_{static_cast<std::size_t>(max_parallelism)},
    src_{graph_,
         [this, read_next_store = std::move(next_store)](tbb::flow_control& fc) mutable -> message {
           auto store = read_next_store();
           if (not store) {
             drain();
             fc.stop();
             return {};
           }
           assert(not store->is_flush());
           return sender_.make_message(accept(std::move(store)));
         }},
    multiplexer_{graph_}
  {
    // FIXME: Should the loading of env levels happen in the meld app only?
    spdlog::cfg::load_env_levels();
    spdlog::info("Number of worker threads: {}",
                 concurrency::max_allowed_parallelism::active_value());

    // The parent of the job message is null
    eoms_.push(nullptr);
  }

  framework_graph::~framework_graph() = default;

  std::size_t framework_graph::execution_counts(std::string const& node_name) const
  {
    // FIXME: Yuck!
    if (auto it = nodes_.filters_.find(node_name); it != nodes_.filters_.end()) {
      return it->second->num_calls();
    }
    if (auto it = nodes_.monitors_.find(node_name); it != nodes_.monitors_.end()) {
      return it->second->num_calls();
    }
    if (auto it = nodes_.reductions_.find(node_name); it != nodes_.reductions_.end()) {
      return it->second->num_calls();
    }
    if (auto it = nodes_.splitters_.find(node_name); it != nodes_.splitters_.end()) {
      return it->second->num_calls();
    }
    if (auto it = nodes_.transforms_.find(node_name); it != nodes_.transforms_.end()) {
      return it->second->num_calls();
    }
    return 0u;
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
    if (not empty(registration_errors_)) {
      std::string error_msg{"\nConfiguration errors:\n"};
      for (auto const& error : registration_errors_) {
        error_msg += "  - " + error + '\n';
      }
      throw std::runtime_error(error_msg);
    }

    filter_collectors_.merge(internal_edges_for_filters(graph_, nodes_.filters_, nodes_.filters_));
    filter_collectors_.merge(internal_edges_for_filters(graph_, nodes_.filters_, nodes_.monitors_));
    filter_collectors_.merge(internal_edges_for_filters(graph_, nodes_.filters_, nodes_.outputs_));
    filter_collectors_.merge(
      internal_edges_for_filters(graph_, nodes_.filters_, nodes_.reductions_));
    filter_collectors_.merge(
      internal_edges_for_filters(graph_, nodes_.filters_, nodes_.splitters_));
    filter_collectors_.merge(
      internal_edges_for_filters(graph_, nodes_.filters_, nodes_.transforms_));

    edge_maker make_edges{dot_file_name, nodes_.outputs_, nodes_.transforms_, nodes_.reductions_};
    make_edges(src_,
               multiplexer_,
               filter_collectors_,
               consumers{nodes_.filters_, {.shape = "box"}},
               consumers{nodes_.monitors_, {.shape = "ellipse"}},
               consumers{nodes_.reductions_, {.arrowtail = "dot", .shape = "ellipse"}},
               consumers{nodes_.splitters_, {.shape = "trapezium"}},
               consumers{nodes_.transforms_, {.shape = "ellipse"}});
  }

  product_store_ptr framework_graph::accept(product_store_ptr store)
  {
    assert(store);
    auto const new_depth = store->id()->depth();
    while (not empty(levels_) and new_depth <= levels_.top().depth()) {
      levels_.pop();
      eoms_.pop();
    }
    levels_.emplace(counters_, sender_, store);
    return store;
  }

  void framework_graph::drain()
  {
    while (not empty(levels_)) {
      levels_.pop();
      eoms_.pop();
    }
  }
}
