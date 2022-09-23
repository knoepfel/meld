#ifndef meld_core_multiplexer_hpp
#define meld_core_multiplexer_hpp

#include "meld/core/message.hpp"
#include "meld/graph/transition.hpp"

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
    };
    struct named_input_port {
      std::string node_name;
      tbb::flow::receiver<message>* port;
    };
    using head_nodes_t = std::map<std::string, named_input_port>;

    explicit multiplexer(tbb::flow::graph& g, bool debug = false) :
      base{g, tbb::flow::unlimited, std::bind_front(&multiplexer::multiplex, this)}, debug_{debug}
    {
    }

    tbb::flow::continue_msg multiplex(message const& msg);

    void finalize(head_nodes_t head_nodes) { head_nodes_ = std::move(head_nodes); }

  private:
    head_nodes_t head_nodes_;
    tbb::concurrent_hash_map<level_id, std::set<tbb::flow::receiver<message>*>> flushes_required_;
    bool debug_;
  };

}

#endif /* meld_core_multiplexer_hpp */
