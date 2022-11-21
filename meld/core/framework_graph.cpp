#include "meld/core/framework_graph.hpp"

#include "meld/concurrency.hpp"
#include "meld/core/edge_maker.hpp"
#include "meld/core/product_store.hpp"

#include "spdlog/cfg/env.h"

namespace {
  meld::level_counter counter;
}

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

  // FIXME 1: The implementation below automatically calculates when flush stores need to
  //          be sent through the graph.  Consequently, it is **slow**.  At the very
  //          least, we should have an overload that allows users to supply all of the
  //          flush values by hand.  This can be very prone to error, however.
  //
  // FIXME 2: The scheduling algorithm below needs to be tested.
  framework_graph::framework_graph(std::function<product_store_ptr()> f) :
    src_{graph_,
         [this, user_function = move(f)](tbb::flow_control& fc) mutable -> message {
           if (shutdown_) {
             if (auto pending_store = next_pending_store()) {
               ++calls_;
               auto h = original_message_ids_.extract(pending_store->id().parent());
               assert(h);
               std::size_t const original_message_id = h.mapped();
               // spdlog::debug("Mark 1: {}", ::to_string(pending_store));
               return {pending_store, calls_, original_message_id};
             }

             fc.stop();
             return {};
           }

           if (not store_) {
             auto store = user_function();
             // spdlog::trace("New store: {}", ::to_string(store));
             shutdown_ = store == nullptr;
             auto next_level = shutdown_ ? level_id::base() : store->id();
             if (next_level != level_id::base()) {
               next_level = next_level.parent();
             }

             store_ = move(store);
             for (auto&& tr : transitions_between(last_id_, next_level, counter)) {
               // spdlog::trace("New transition: {} {}", tr.first, to_string(tr.second));
               pending_transitions_.push(move(tr));
             }
             if (shutdown_ and next_level == level_id::base()) {
               pending_transitions_.push({counter.value_as_id(level_id::base()), stage::flush});
             }
           }

           ++calls_;
           if (auto pending_store = next_pending_store()) {
             if (pending_store->is_flush()) {
               // Original message ID no longer needed after this message.
               auto h = original_message_ids_.extract(pending_store->id().parent());
               assert(h);
               std::size_t const original_message_id = h.mapped();
               // spdlog::debug("Mark 2: {}", ::to_string(pending_store));
               return {move(pending_store), calls_, original_message_id};
             }
             original_message_ids_.try_emplace(pending_store->id(), calls_);
             // spdlog::debug("Mark 3: {}", ::to_string(pending_store));
             return {pending_store, calls_};
           }

           assert(store_);
           last_id_ = store_->id();
           if (store_->is_flush()) {
             // Original message ID no longer needed after this message.
             auto h = original_message_ids_.extract(store_->id().parent());
             assert(h);
             std::size_t const original_message_id = h.mapped();
             // spdlog::debug("Mark 4: {}", ::to_string(store_));
             return {move(store_), calls_, original_message_id};
           }

           // spdlog::debug("Mark 5: {}", ::to_string(store_));
           original_message_ids_.try_emplace(store_->id(), calls_);
           return {move(store_), calls_};
         }},
    multiplexer_{graph_}
  {
    spdlog::cfg::load_env_levels();
  }

  void framework_graph::execute(std::string const& dot_file_name)
  {
    spdlog::info("Number of worker threads: {}",
                 concurrency::max_allowed_parallelism::active_value());
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

  product_store_ptr framework_graph::next_pending_store()
  {
    if (empty(pending_transitions_)) {
      return nullptr;
    }

    auto [id, stage] = pending_transitions_.front();
    pending_transitions_.pop();
    return make_product_store(std::move(id), "Source", stage);
  }
}
