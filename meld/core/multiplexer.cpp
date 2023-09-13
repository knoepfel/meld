#include "meld/core/multiplexer.hpp"
#include "meld/model/product_store.hpp"

#include "oneapi/tbb/flow_graph.h"
#include "spdlog/spdlog.h"

#include <algorithm>

using namespace std::chrono;

namespace meld {

  multiplexer::multiplexer(tbb::flow::graph& g, bool debug) :
    base{g, tbb::flow::unlimited, std::bind_front(&multiplexer::multiplex, this)}, debug_{debug}
  {
  }

  void multiplexer::finalize(head_ports_t head_ports) { head_ports_ = std::move(head_ports); }

  tbb::flow::continue_msg multiplexer::multiplex(message const& msg)
  {
    ++received_messages_;
    auto const& [store, eom, message_id] = std::tie(msg.store, msg.eom, msg.id);
    if (debug_) {
      spdlog::debug("Multiplexing {} with ID {} (is flush: {})",
                    store->id()->to_string(),
                    message_id,
                    store->is_flush());
    }
    auto start_time = steady_clock::now();

    if (store->is_flush()) {
      for (auto const& head_port : head_ports_) {
        head_port.port->try_put(msg);
      }
      return {};
    }

    for (auto const& head_port : head_ports_) {
      auto store_to_send = store->store_for_product(head_port.product_name);
      if (not store_to_send) {
        // This can be fine if the store is not expected to contain the product.  However,
        // this can be a problem if the user specifies which store should contain the
        // product.
        continue;
      }

      if (auto allowed_domain = head_port.domain) {
        if (store_to_send->level_name() != *allowed_domain) {
          continue;
        }
      }
      head_port.port->try_put({store_to_send, eom, message_id});
    }

    execution_time_ += duration_cast<microseconds>(steady_clock::now() - start_time);
    return {};
  }

  multiplexer::~multiplexer()
  {
    spdlog::debug("Routed {} messages in {} microseconds ({:.3f} microseconds per message)",
                  received_messages_,
                  execution_time_.count(),
                  execution_time_.count() / received_messages_);
  }
}
