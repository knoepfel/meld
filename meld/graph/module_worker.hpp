#ifndef meld_graph_module_worker_hpp
#define meld_graph_module_worker_hpp

#include "meld/core/concurrency_tags.hpp"
#include "meld/graph/data_node.hpp"
#include "meld/graph/serial_node.hpp"

#include "boost/json.hpp"
#include "oneapi/tbb/flow_graph.h"

#include <map>
#include <memory>
#include <optional>

namespace meld {
  using module_node = base<data_node_ptr>;

  class module_worker {
  public:
    virtual ~module_worker();

    std::vector<transition_type> supported_transitions() const;
    std::vector<std::string> dependencies() const;
    std::shared_ptr<module_node> create_worker_node(tbb::flow::graph& graph,
                                                    transition_type const& tt,
                                                    serializers& serialized_resources);

  private:
    virtual std::shared_ptr<module_node> do_create_worker_node(
      tbb::flow::graph& graph, transition_type const& tt, serializers& serialized_resources) = 0;
    virtual std::vector<std::string> required_dependencies() const = 0;
    virtual std::vector<transition_type> supported_setup_transitions() const = 0;
    virtual std::vector<transition_type> supported_process_transitions() const = 0;
  };

  using module_worker_ptr = std::unique_ptr<module_worker>;
  using module_creator_t = module_worker_ptr(boost::json::value const&);
}

#endif /* meld_graph_module_worker_hpp */
