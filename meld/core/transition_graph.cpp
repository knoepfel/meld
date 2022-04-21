#include "meld/core/transition_graph.hpp"

namespace meld {
  transition_graph::transition_graph(stage const s) : stage_{s} {}

  void
  transition_graph::add_node(std::string const& name, module_worker& worker)
  {
    nodes_.try_emplace(name, module_node{graph_, tbb::flow::serial, [&worker, this](node_ptr n) {
                                           worker.process(stage_, *n);
                                           return n;
                                         }});
    module_dependencies_.try_emplace(name, worker.dependencies());
  }

  void
  transition_graph::calculate_edges()
  {
    if (empty(nodes_))
      return;

    for (auto const& [name, deps] : module_dependencies_) {
      auto& mod_node = nodes_.at(name);
      if (empty(deps)) {
        make_edge(launcher_, mod_node);
        continue;
      }

      for (auto const& dep : deps) {
        make_edge(nodes_.at(dep), mod_node);
      }
    }
  }

  void
  transition_graph::process(node_ptr n)
  {
    if (empty(nodes_))
      return;

    launcher_.try_put(n);
    graph_.wait_for_all();
  }
}
