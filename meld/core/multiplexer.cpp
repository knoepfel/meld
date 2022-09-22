#include "meld/core/multiplexer.hpp"
#include "meld/core/product_store.hpp"

#include "oneapi/tbb/flow_graph.h"

namespace meld {

  tbb::flow::continue_msg
  multiplexer::multiplex(message const& msg)
  {
    auto const& [store, message_id, _] = msg;
    if (debug_) {
      debug("Multiplexing ", store->id(), " with ID ", message_id);
    }
    using accessor = decltype(flushes_required_)::accessor;
    if (store->is_flush()) {
      accessor a;
      bool const found = flushes_required_.find(a, store->parent()->id());
      if (!found) {
        a.release();
        // FIXME: This is the case where (a) no nodes exist, or (b)
        // the flush message has been received before any other
        // messages.  In case (b), we send the flush message to all
        // nodes as we do not yet know which nodes will process a
        // given processing level.  For case (a), ideally we would
        // find a better solution.
        for (auto const& [_, node] : head_nodes_) {
          node.port->try_put(msg);
        }
        return {};
      }

      for (auto port : a->second) {
        port->try_put(msg);
      }
      flushes_required_.erase(a);
      return {};
    }

    // TODO: Add option to send directly to any functions that specify
    // they are of a certain processing level.
    for (auto const& [key, store_ptr] : store->stores_for_products()) {
      if (auto it = head_nodes_.find(key); it != cend(head_nodes_)) {
        auto store_to_send = store_ptr.lock();
        it->second.port->try_put({store_to_send, message_id});
        if (auto& parent = store_to_send->parent()) {
          accessor a;
          flushes_required_.insert(a, parent->id());
          a->second.insert(it->second.port);
        }
      }
    }
    return {};
  }
}
