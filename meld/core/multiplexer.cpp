#include "meld/core/multiplexer.hpp"

#include "meld/core/product_store.hpp"

#include "oneapi/tbb/flow_graph.h"

namespace meld {

  tbb::flow::continue_msg
  multiplexer::multiplex(message const& msg)
  {
    auto const& [store, message_id, _] = msg;
    // debug("Multiplexing ", store->id(), " with ID ", message_id);
    using accessor = decltype(flushes_required_)::accessor;
    if (store->is_flush()) {
      accessor a;
      bool const found = flushes_required_.find(a, store->parent()->id());
      if (!found) {
        // FIXME: This is the case where no nodes exist.  Should
        // probably either detect this situation and warn or....?
        return {};
      }
      for (auto node : a->second) {
        node->try_put(msg);
      }
      flushes_required_.erase(a);
      return {};
    }

    // TODO: Add option to send directly to any functions that specify
    // they are of a certain processing level.
    for (auto const& [key, store_ptr] : store->stores_for_products()) {
      if (auto it = head_nodes_.find(key); it != cend(head_nodes_)) {
        auto store_to_send = store_ptr.lock();
        it->second->try_put({store_to_send, message_id});
        if (auto& parent = store_to_send->parent()) {
          accessor a;
          flushes_required_.insert(a, parent->id());
          a->second.insert(it->second);
        }
      }
    }
    return {};
  }

}
