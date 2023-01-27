#include "meld/core/framework_graph.hpp"

#include "meld/concurrency.hpp"
#include "meld/core/edge_maker.hpp"
#include "meld/model/level_counter.hpp"
#include "meld/model/product_store.hpp"

#include "spdlog/cfg/env.h"

namespace {
  meld::level_counter counter;

}

namespace meld {
  level_sentry::level_sentry(std::queue<product_store_ptr>& pending_stores,
                             product_store_ptr store) :
    pending_stores_{pending_stores}, store_{move(store)}
  {
    if (id().has_parent()) {
      counter.record_parent(id());
    }
    pending_stores_.push(store_);
  }

  level_sentry::~level_sentry()
  {
    pending_stores_.push(store_->make_flush(counter.value_as_id(id())));
  }

  level_id const& level_sentry::id() const { return store_->id(); }

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
         [this, read_next_store = move(next_store)](tbb::flow_control& fc) mutable -> message {
           if (auto store = pending_store()) {
             return send(store);
           }

           if (shutdown_) {
             if (auto store = pending_store()) {
               return send(store);
             }
             fc.stop();
             return {};
           }

           auto store = read_next_store();
           if (not store) {
             shutdown_ = true;
             drain();
           }
           else {
             accept(move(store));
           }
           return send(pending_store());
         }},
    multiplexer_{graph_}
  {
    // FIXME: Should the loading of env levels happen in the meld app only?
    spdlog::cfg::load_env_levels();
    spdlog::info("Number of worker threads: {}",
                 concurrency::max_allowed_parallelism::active_value());
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

    filter_collectors_.merge(internal_edges_for_filters(graph_, filters_, filters_));
    filter_collectors_.merge(internal_edges_for_filters(graph_, filters_, monitors_));
    filter_collectors_.merge(internal_edges_for_filters(graph_, filters_, outputs_));
    filter_collectors_.merge(internal_edges_for_filters(graph_, filters_, reductions_));
    filter_collectors_.merge(internal_edges_for_filters(graph_, filters_, splitters_));
    filter_collectors_.merge(internal_edges_for_filters(graph_, filters_, transforms_));

    edge_maker make_edges{dot_file_name, outputs_, transforms_, reductions_};
    make_edges(src_,
               multiplexer_,
               filter_collectors_,
               consumers{filters_, {.shape = "box"}},
               consumers{monitors_, {.shape = "ellipse"}},
               consumers{reductions_, {.arrowtail = "dot", .shape = "ellipse"}},
               consumers{splitters_, {.shape = "trapezium"}},
               consumers{transforms_, {.shape = "ellipse"}});
  }

  void framework_graph::accept(product_store_ptr store)
  {
    assert(store);
    auto const new_depth = store->id().depth();
    while (not empty(levels_) and new_depth <= levels_.top()->id().depth()) {
      levels_.pop();
    }
    levels_.push(std::make_unique<level_sentry>(pending_stores_, move(store)));
  }

  void framework_graph::drain()
  {
    while (not empty(levels_))
      levels_.pop();
  }

  message framework_graph::send(product_store_ptr store)
  {
    assert(store);
    auto const message_id = ++calls_;
    if (store->is_flush()) {
      return {store, message_id, original_message_id(store)};
    }
    original_message_ids_.try_emplace(store->id(), message_id);
    return {store, message_id, -1ull};
  }

  std::size_t framework_graph::original_message_id(product_store_ptr const& store)
  {
    assert(store);
    assert(store->is_flush());

    auto const& id = store->id();
    if (not id.has_parent()) {
      return 1;
    }

    auto h = original_message_ids_.extract(id.parent());
    assert(h);
    return h.mapped();
  }

  product_store_ptr framework_graph::pending_store()
  {
    if (empty(pending_stores_)) {
      return nullptr;
    }
    auto result = pending_stores_.front();
    pending_stores_.pop();
    return result;
  }
}
