#include "meld/graph/transition_graph.hpp"
#include "meld/graph/gatekeeper_node.hpp"
#include "meld/utilities/debug.hpp"

#include "oneapi/tbb/flow_graph.h"

using namespace tbb;

namespace meld {
  transition_graph::transition_graph(flow::graph& g, stage const s) :
    graph_{g},
    stage_{s},
    synchronize_{g, flow::unlimited, [this](data_node_ptr n, auto& outputs) mutable {
                   if (decltype(counters_)::accessor a; counters_.find(a, n->id())) {
                     if (--a->second == 0u) {
                       transition tr{n->id(), stage_};
                       std::get<0>(outputs).try_put({tr, n});
                       counters_.erase(a);
                     }
                   }
                 }}
  {
  }

  void
  transition_graph::add_node(std::string const& name,
                             transition_type const& tt,
                             module_worker& worker,
                             serializers& serialized_resources)
  {
    nodes_.emplace(name, worker.create_worker_node(graph_, tt, serialized_resources));
    module_dependencies_.try_emplace(name, worker.dependencies());
  }

  void
  transition_graph::calculate_edges(gatekeeper_node& gatekeeper)
  {
    if (empty(nodes_))
      return;

    std::map<std::string, unsigned> successors;
    for (auto const& [name, deps] : module_dependencies_) {
      auto& mod_node = nodes_.at(name);
      successors[name] = 0;
      if (empty(deps)) {
        make_edge(launcher_, *mod_node);
        continue;
      }

      for (auto const& dep : deps) {
        make_edge(*nodes_.at(dep), *mod_node);
        ++successors[dep];
      }
    }

    // Figure out how many end points there are that synchronize must wait for.
    for (auto const& [name, count] : successors) {
      if (count == 0) {
        ++num_end_points_;
        make_edge(*nodes_.at(name), synchronize_);
      }
    }

    make_edge(output_port<0>(synchronize_), input_port<1>(gatekeeper));
  }

  void
  transition_graph::process(data_node_ptr n)
  {
    if (empty(nodes_))
      return;

    assert(n);
    counters_.emplace(n->id(), num_end_points_);
    launcher_.try_put(n);
  }
}
