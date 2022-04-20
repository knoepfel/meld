#include "sand/core/transition_graph.hpp"

namespace sand {
  transition_graph::transition_graph(stage const s) : stage_{s} {}

  void
  transition_graph::add_node(std::string const& name, module_worker& worker)
  {
    nodes_.try_emplace(name, module_node{graph_, tbb::flow::serial, [&worker, this](node_ptr n) {
                                           worker.process(stage_, *n);
                                           return n;
                                         }});
  }

  void
  transition_graph::calculate_edges()
  {
    if (empty(nodes_))
      return;

    auto first = begin(nodes_);
    make_edge(launcher_, first->second);
    for (auto a = first, b = next(a); b != end(nodes_); ++a, ++b) {
      make_edge(a->second, b->second);
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
