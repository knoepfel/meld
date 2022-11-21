#include "meld/core/multiplexer.hpp"
#include "meld/core/product_store.hpp"

#include "oneapi/tbb/flow_graph.h"
#include "spdlog/spdlog.h"

namespace meld {

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
        it->second.port->try_put({store_to_send, message_id});
      }
    }
    return {};
  }
}
