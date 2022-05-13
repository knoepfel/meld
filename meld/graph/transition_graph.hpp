#ifndef meld_graph_transition_graph_hpp
#define meld_graph_transition_graph_hpp

#include "meld/graph/data_node.hpp"
#include "meld/graph/module_worker.hpp"
#include "meld/graph/serializer_node.hpp"
#include "meld/graph/transition.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <map>
#include <vector>

namespace meld {
  class gatekeeper_node;
  class transition_graph {
  public:
    explicit transition_graph(tbb::flow::graph& g, stage s);
    void process(data_node_ptr n);
    void add_node(std::string const& name,
                  transition_type const& tt,
                  module_worker& worker,
                  serializers& serialized_resources);
    void calculate_edges(gatekeeper_node& gatekeeper);

  private:
    tbb::flow::graph& graph_;
    stage stage_;
    tbb::flow::broadcast_node<data_node_ptr> launcher_{graph_};
    std::map<std::string, std::shared_ptr<module_node>> nodes_{};
    std::map<std::string, std::vector<std::string>> module_dependencies_{};
    unsigned num_end_points_{};
    tbb::concurrent_hash_map<level_id, unsigned, IDHasher> counters_{};
    tbb::flow::multifunction_node<data_node_ptr, std::tuple<transition_message>> synchronize_;
  };
}

#endif /* meld_graph_transition_graph_hpp */
