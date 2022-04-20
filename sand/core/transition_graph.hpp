#ifndef sand_core_transition_graph_hpp
#define sand_core_transition_graph_hpp

#include "sand/core/module_worker.hpp"
#include "sand/core/node.hpp"
#include "sand/core/transition.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <map>
#include <vector>

namespace sand {

  class transition_graph {
  public:
    explicit transition_graph(stage s);
    void process(node_ptr n);
    void add_node(std::string const& name, module_worker& worker);
    void calculate_edges();

  private:
    using module_node = tbb::flow::function_node<node_ptr, node_ptr>;
    stage stage_;
    tbb::flow::graph graph_{};
    tbb::flow::broadcast_node<node_ptr> launcher_{graph_};
    std::map<std::string, module_node> nodes_{};
  };
}

#endif /* sand_core_transition_graph_hpp */
