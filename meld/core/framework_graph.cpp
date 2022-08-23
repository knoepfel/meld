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
  std::string const source_name{"[source]"};

  std::string
  join_name_for(std::string const& name)
  {
    return "[join for " + name + ']';
  }

  std::string const&
  sender_name_for_product(std::map<std::string, std::vector<std::string>> const& available_products,
                          std::string const& label)
  {
    if (auto it = available_products.find(label); it != cend(available_products)) {
      // FIXME:  THIS LOOKUP ASSUMES ONLY ONE FUNCTION PROVIDES THE PRODUCT
      auto const& candidate_fcns = it->second;
      return candidate_fcns[0];
    }
    return source_name;
  }
}

namespace meld {

  framework_graph::framework_graph(run_once_t, product_store_ptr store) :
    src_{graph_, [store, executed = false](auto& fc) mutable -> product_store_ptr {
           if (executed) {
             fc.stop();
             return {};
           }
           executed = true;
           return store;
         }}
  {
  }

  void
  framework_graph::merge(declared_transforms&& user_fcns, declared_reductions&& user_reductions)
  {
    funcs_.merge(std::move(user_fcns));
    reductions_.merge(std::move(user_reductions));
  }

  void
  framework_graph::finalize_and_run()
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

  framework_graph::user_function_node
  framework_graph::node_for(declared_transform_ptr& ptr)
  {
    return user_function_node{graph_, ptr->concurrency(), [&ptr](product_store_ptr const& store) {
                                ptr->invoke(*store);
                                return store;
                              }};
  }

  framework_graph::reduction_node
  framework_graph::node_for(declared_reduction_ptr& ptr)
  {
    return reduction_node{
      graph_, ptr->concurrency(), [&ptr](product_store_ptr const& store, auto& outputs) {
        ptr->invoke(*store);
        if (auto const& parent = store->parent(); ptr->reduction_complete(*parent)) {
          get<0>(outputs).try_put(parent);
        }
      }};
  }

  void
  framework_graph::finalize()
  {
    for (auto& [name, p] : reductions_) {
      joins_.try_emplace(join_name_for(name), dynamic_join{graph_, ProductStoreHasher{}});
      reduction_nodes_.try_emplace(name, node_for(p));
    }

    for (auto& [name, p] : funcs_) {
      joins_.try_emplace(join_name_for(name), dynamic_join{graph_, ProductStoreHasher{}});
      transform_nodes_.try_emplace(name, node_for(p));
    }

    std::set<std::pair<std::string, std::string>> existing_edges;
    auto const available_products = produced_products();
    for (auto const& [fcn_name, product_labels] : consumed_products()) {
      auto& fcn_node = receiver_for(fcn_name);
      for (auto const& label : product_labels) {
        auto const& sender_name = sender_name_for_product(available_products, label);
        if (existing_edges.contains({sender_name, fcn_name})) {
          continue;
        }
        make_edge(sender_for(sender_name), join_for(fcn_name));
        make_edge(join_for(fcn_name), fcn_node);
        existing_edges.insert({sender_name, fcn_name});
      }
    }
  }

  tbb::flow::sender<product_store_ptr>&
  framework_graph::sender_for(std::string const& name)
  {
    if (name == source_name) {
      return src_;
    }
    if (auto it = transform_nodes_.find(name); it != cend(transform_nodes_)) {
      return it->second;
    }
    return output_port<0>(reduction_nodes_.at(name));
  }

  tbb::flow::receiver<product_store_ptr>&
  framework_graph::receiver_for(std::string const& name)
  {
    if (auto it = transform_nodes_.find(name); it != cend(transform_nodes_)) {
      return it->second;
    }
    return reduction_nodes_.at(name);
  }

  framework_graph::dynamic_join&
  framework_graph::join_for(std::string const& name)
  {
    return joins_.at(join_name_for(name));
  }

  std::map<std::string, std::vector<std::string>>
  framework_graph::produced_products() const
  {
    // [product name] <=> [function names]
    std::map<std::string, std::vector<std::string>> result;
    for (auto const& [_, function] : funcs_) {
      for (auto const& label : function->output()) {
        result[label].push_back(function->name());
      }
    }
    for (auto const& [_, function] : reductions_) {
      for (auto const& label : function->output()) {
        result[label].push_back(function->name());
      }
    }
    return result;
  }

  std::map<std::string, std::vector<std::string>>
  framework_graph::consumed_products() const
  {
    // [function name] <=> [product names]
    std::map<std::string, std::vector<std::string>> result;
    for (auto const& [_, function] : funcs_) {
      result.try_emplace(function->name(), function->input());
    }
    for (auto const& [_, function] : reductions_) {
      result.try_emplace(function->name(), function->input());
    }
    return result;
  }
}
