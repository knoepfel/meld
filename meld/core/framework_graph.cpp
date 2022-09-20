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
    meld::multiplexer::head_nodes_t result;
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
    auto head_nodes = make_edges(nodes_that_produce, transforms_);
    head_nodes.merge(make_edges(nodes_that_produce, reductions_));
    head_nodes.merge(make_edges(nodes_that_produce, splitters_));

    // Create head nodes for splitters
    auto get_consumed_products = [](auto const& consumers, auto& products) {
      for (auto const& [key, consumer] : consumers) {
        for (auto const& product_name : consumer->input()) {
          products[product_name].push_back(key);
        }
      }
    };

    std::map<std::string, std::vector<std::string>> consumed_products;
    get_consumed_products(transforms_, consumed_products);
    get_consumed_products(reductions_, consumed_products);
    get_consumed_products(splitters_, consumed_products);

    std::set<std::string> head_nodes_to_remove;
    for (auto const& [name, splitter] : splitters_) {
      multiplexer::head_nodes_t heads;
      for (auto const& product_name : splitter->provided_products()) {
        heads.insert(*head_nodes.find(product_name));
        head_nodes_to_remove.insert(product_name);
      }
      splitter->finalize(std::move(heads));
    }

    // Remove head nodes claimed by splitters
    for (auto const& key : head_nodes_to_remove) {
      head_nodes.erase(key);
    }

    multiplexer_.finalize(move(head_nodes));
    make_edge(src_, multiplexer_);
  }
}
