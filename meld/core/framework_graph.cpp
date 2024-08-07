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
    framework_graph{[store](cached_product_stores&) mutable {
                      // Returns non-null store, then replaces it with nullptr, thus
                      // resulting in one graph execution.
                      return std::exchange(store, nullptr);
                    },
                    max_parallelism}
  {
  }

  framework_graph::framework_graph(std::function<product_store_ptr()> f,
                                   int const max_parallelism) :
    framework_graph{[ft = std::move(f)](cached_product_stores&) mutable { return ft(); },
                    max_parallelism}
  {
  }

  // FIXME: The algorithm below should support user-specified flush stores.
  framework_graph::framework_graph(detail::next_store_t next_store, int const max_parallelism) :
    parallelism_limit_{static_cast<std::size_t>(max_parallelism)},
    src_{graph_,
         [this, read_next = std::move(next_store)](tbb::flow_control& fc) mutable -> message {
           auto store = read_next(stores_);
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
    if (auto it = nodes_.predicates_.find(node_name); it != nodes_.predicates_.end()) {
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

  std::size_t framework_graph::product_counts(std::string const& node_name) const
  {
    // FIXME: Yuck!
    if (auto it = nodes_.reductions_.find(node_name); it != nodes_.reductions_.end()) {
      return it->second->product_count();
    }
    if (auto it = nodes_.splitters_.find(node_name); it != nodes_.splitters_.end()) {
      return it->second->product_count();
    }
    if (auto it = nodes_.transforms_.find(node_name); it != nodes_.transforms_.end()) {
      return it->second->product_count();
    }
    return 0u;
  }

  void framework_graph::execute(std::string const& dot_file_prefix)
  {
    finalize(dot_file_prefix);
    run();
    // post_data_graph(dot_file_prefix);
  }

  void framework_graph::run()
  {
    src_.activate();
    graph_.wait_for_all();
  }

  namespace {
    template <typename T>
    auto internal_edges_for_predicates(oneapi::tbb::flow::graph& g,
                                       declared_predicates& all_predicates,
                                       T const& consumers)
    {
      std::map<std::string, filter> result;
      for (auto const& [name, consumer] : consumers) {
        auto const& predicates = consumer->when();
        if (empty(predicates)) {
          continue;
        }

        auto [it, success] = result.try_emplace(name, g, *consumer);
        for (auto const& predicate_name : predicates) {
          auto fit = all_predicates.find(predicate_name);
          if (fit == cend(all_predicates)) {
            throw std::runtime_error("A non-existent filter with the name '" + predicate_name +
                                     "' was specified for " + name);
          }
          make_edge(fit->second->sender(), it->second.predicate_port());
        }
      }
      return result;
    }
  }

  void framework_graph::finalize(std::string const& dot_file_prefix)
  {
    if (not empty(registration_errors_)) {
      std::string error_msg{"\nConfiguration errors:\n"};
      for (auto const& error : registration_errors_) {
        error_msg += "  - " + error + '\n';
      }
      throw std::runtime_error(error_msg);
    }

    filters_.merge(internal_edges_for_predicates(graph_, nodes_.predicates_, nodes_.predicates_));
    filters_.merge(internal_edges_for_predicates(graph_, nodes_.predicates_, nodes_.monitors_));
    filters_.merge(internal_edges_for_predicates(graph_, nodes_.predicates_, nodes_.outputs_));
    filters_.merge(internal_edges_for_predicates(graph_, nodes_.predicates_, nodes_.reductions_));
    filters_.merge(internal_edges_for_predicates(graph_, nodes_.predicates_, nodes_.splitters_));
    filters_.merge(internal_edges_for_predicates(graph_, nodes_.predicates_, nodes_.transforms_));

    edge_maker make_edges{dot_file_prefix, nodes_.outputs_, nodes_.transforms_, nodes_.reductions_};
    make_edges(src_,
               multiplexer_,
               filters_,
               consumers{nodes_.predicates_, {.shape = "box"}},
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
