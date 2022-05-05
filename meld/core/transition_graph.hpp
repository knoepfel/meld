#ifndef meld_core_transition_graph_hpp
#define meld_core_transition_graph_hpp

#include "meld/core/module_worker.hpp"
#include "meld/core/node.hpp"
#include "meld/core/transition.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <map>
#include <vector>

namespace meld {

  class transition_graph {
  public:
    explicit transition_graph(stage s);
    void process(node_ptr n);
    void add_node(std::string const& name, transition_type const& tt, module_worker& worker);
    void calculate_edges();

  private:
    using module_node = tbb::flow::function_node<node_ptr, node_ptr>;
    stage stage_;
    tbb::flow::graph graph_{};
    tbb::flow::broadcast_node<node_ptr> launcher_{graph_};
    std::map<std::string, module_node> nodes_{};
    std::map<std::string, std::vector<std::string>> module_dependencies_{};
  };
}

#endif /* meld_core_transition_graph_hpp */
