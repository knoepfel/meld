#ifndef meld_core_multiplexer_hpp
#define meld_core_multiplexer_hpp

#include "meld/core/message.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <functional>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace meld {

  class multiplexer : public tbb::flow::function_node<message> {
    using base = tbb::flow::function_node<message>;

  public:
    using head_nodes_t = std::map<std::string, tbb::flow::receiver<message>*>;
    explicit multiplexer(tbb::flow::graph& g) :
      base{g, tbb::flow::unlimited, std::bind_front(&multiplexer::multiplex, this)}
    {
    }

    tbb::flow::continue_msg multiplex(message const& msg);

    void
    finalize(head_nodes_t head_nodes)
    {
      head_nodes_ = std::move(head_nodes);
    }

  private:
    head_nodes_t head_nodes_;
    tbb::concurrent_hash_map<level_id, std::set<tbb::flow::receiver<message>*>> flushes_required_;
  };

}

#endif /* meld_core_multiplexer_hpp */
