#include "meld/core/multiplexer.hpp"
#include "meld/model/product_store.hpp"

#include "oneapi/tbb/flow_graph.h"
#include "spdlog/spdlog.h"

#include <algorithm>

namespace meld {

  multiplexer::multiplexer(tbb::flow::graph& g, bool debug) :
    base{g, tbb::flow::unlimited, std::bind_front(&multiplexer::multiplex, this)}, debug_{debug}
  {
  }

  void multiplexer::finalize(head_nodes_t head_nodes) { head_nodes_ = std::move(head_nodes); }

  tbb::flow::continue_msg multiplexer::multiplex(message const& msg)
  {
    auto const& [store, message_id] = std::tie(msg.store, msg.id);
    if (debug_) {
      spdlog::debug("Multiplexing {} with ID {}", store->id(), message_id);
    }
    if (store->is_flush()) {
      for (auto const& [_, node] : head_nodes_) {
        node.port->try_put(msg);
      }
      return {};
    }

    // TODO: Add option to send directly to any functions that specify they are of a
    //       certain processing level.
    for (auto const& [key, store_ptr] : store->stores_for_products()) {
      auto store_to_send = store_ptr.lock();
      for (auto [it, e] = head_nodes_.equal_range(key); it != e; ++it) {
        auto const& named_port = it->second;
        // If 'when_in' has been specified, only send if the store's level name matches
        // the when_in specification.
        if (auto store_names = named_port.accepts_stores) {
          if (not std::binary_search(
                begin(*store_names), end(*store_names), store_to_send->level_name())) {
            continue;
          }
        }
        named_port.port->try_put({store_to_send, message_id});
      }
    }
    return {};
  }
}
