#include "meld/core/multiplexer.hpp"
#include "meld/model/product_store.hpp"

#include "oneapi/tbb/flow_graph.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <stdexcept>

using namespace std::chrono;

namespace {
  meld::product_store_const_ptr store_for(meld::product_store_const_ptr store,
                                          meld::specified_label const& label)
  {
    auto const& [product_name, domain, relation] = label;
    if (domain.empty()) {
      return store->store_for_product(product_name);
    }
    if (store->level_name() == domain and store->contains_product(product_name)) {
      return store;
    }
    auto parent = store->parent(domain);
    if (not parent) {
      return nullptr;
    }
    if (parent->contains_product(product_name)) {
      return parent;
    }
    throw std::runtime_error(
      fmt::format("Store not available that provides product {}", label.to_string()));
  }

  struct sender_slot {
    tbb::flow::receiver<meld::message>* port;
    meld::product_store_const_ptr store;
    void operator()(meld::end_of_message_ptr eom, std::size_t message_id) const
    {
      port->try_put({store, eom, message_id});
    }
  };

  std::vector<sender_slot> senders_for(meld::product_store_const_ptr store,
                                       meld::multiplexer::named_input_ports_t const& ports)
  {
    std::vector<sender_slot> result;
    result.reserve(ports.size());
    for (auto const& [product_label, port] : ports) {
      auto store_to_send = store_for(store, product_label);
      if (not store_to_send) {
        // This is fine if the store is not expected to contain the product.
        continue;
      }

      if (auto const& allowed_domain = product_label.domain; not allowed_domain.empty()) {
        if (store_to_send->level_name() != allowed_domain) {
          continue;
        }
      }

      result.push_back({port, store_to_send});
    }
    return result;
  }
}

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
      for (auto const& [_, head_ports] : head_ports_) {
        for (auto const& head_port : head_ports) {
          head_port.port->try_put(msg);
        }
      }
      return {};
    }

    for (auto const& [node_name, ports] : head_ports_) {
      // FIXME: Should make sure that the received store has a level equal to the most
      //        derived store required by the algorithm.
      auto const senders = senders_for(store, ports);
      if (size(senders) != size(ports)) {
        // Not enough stores to ports of the node
        continue;
      }

      for (auto const& sender : senders) {
        sender(eom, message_id);
      }
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
