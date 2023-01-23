#ifndef meld_core_multiplexer_hpp
#define meld_core_multiplexer_hpp

#include "meld/core/message.hpp"
#include "meld/model/transition.hpp"

#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/flow_graph.h"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>

namespace meld {

  class multiplexer : public tbb::flow::function_node<message> {
    using base = tbb::flow::function_node<message>;

  public:
    struct named_output_port {
      std::string node_name;
      tbb::flow::sender<message>* port;
      tbb::flow::sender<message>* to_output;
    };
    struct named_input_port {
      std::string node_name;
      std::vector<std::string> const* accepts_stores;
      tbb::flow::receiver<message>* port;
    };
    using head_nodes_t = std::multimap<std::string, named_input_port>;

    explicit multiplexer(tbb::flow::graph& g, bool debug = false);
    tbb::flow::continue_msg multiplex(message const& msg);

    void finalize(head_nodes_t head_nodes);

  private:
    head_nodes_t head_nodes_;
    bool debug_;
  };

}

#endif /* meld_core_multiplexer_hpp */
