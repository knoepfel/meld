#include "meld/core/framework_graph.hpp"

#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/dynamic_join_node.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {
  using product_name_t = std::string;

  template <typename T>
  auto
  producing_nodes(T& nodes)
  {
    std::map<product_name_t, tbb::flow::sender<meld::message>*> result;
    for (auto const& [node_name, node] : nodes) {
      for (auto const& product_name : node->output()) {
        if (empty(product_name))
          continue;
        result[product_name] = &node->sender();
      }
    }
    return result;
  }

  template <typename T>
  auto
  make_edges(std::map<product_name_t, tbb::flow::sender<meld::message>*> const& producers,
             T& consumers)
  {
    std::map<std::string, tbb::flow::receiver<meld::message>*> result;
    for (auto& [node_name, node] : consumers) {
      for (auto const& product_name : node->input()) {
        auto it = producers.find(product_name);
        if (it != cend(producers)) {
          make_edge(*it->second, node->port(product_name));
        }
        else {
          // Is there a way to detect mis-specified product dependencies?
          result[product_name] = &node->port(product_name);
        }
      }
    }
    return result;
  }
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

  void
  framework_graph::execute()
  {
    finalize();
    run();
  }

  void
  framework_graph::run()
  {
    src_.activate();
    graph_.wait_for_all();
  }

  void
  framework_graph::finalize()
  {
    // Calculate produced products
    auto nodes_that_produce = producing_nodes(transforms_);
    nodes_that_produce.merge(producing_nodes(reductions_));

    // Create edges between nodes per product dependencies
    head_nodes_.merge(make_edges(nodes_that_produce, transforms_));
    head_nodes_.merge(make_edges(nodes_that_produce, reductions_));
    head_nodes_.merge(make_edges(nodes_that_produce, splitters_));
    make_edge(src_, multiplexer_);
  }

  tbb::flow::continue_msg
  framework_graph::multiplex(message const& msg)
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
