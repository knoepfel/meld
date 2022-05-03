#ifndef meld_core_gatekeeper_node_hpp
#define meld_core_gatekeeper_node_hpp

#include "meld/core/node.hpp"
#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/flow_graph.h"

namespace meld {
  namespace detail {
    using msg_2_tuple = std::tuple<transition_message, transition_message>;
    using msg_3_tuple = std::tuple<transition_message, transition_message, transition_message>;
  }

  class gatekeeper_node :
    public tbb::flow::composite_node<detail::msg_2_tuple, detail::msg_2_tuple> {
  public:
    explicit gatekeeper_node(tbb::flow::graph& g);

  private:
    using base_t = tbb::flow::composite_node<detail::msg_2_tuple, detail::msg_2_tuple>;
    using indexer_node = tbb::flow::indexer_node<transition_message, transition_message>;
    using msg_t = indexer_node::output_type;

    bool is_ready(level_id const& id) const;
    bool is_initialized(level_id const& id) const;
    bool is_flush(level_id const& id);

    using multiplexer_node = tbb::flow::multifunction_node<msg_t, detail::msg_3_tuple>;
    using multiplexer_output_ports_type = multiplexer_node::output_ports_type;
    void multiplex(msg_t const& msg, multiplexer_output_ports_type& outputs);

    tbb::concurrent_hash_map<level_id, bool, IDHasher> setup_complete;
    using setup_accessor = decltype(setup_complete)::accessor;
    tbb::concurrent_hash_map<level_id, unsigned, IDHasher> flush_values;
    using accessor = decltype(flush_values)::accessor;
    level_counter counter_;
    indexer_node indexer_;
    multiplexer_node multiplexer_;
  };

}

#endif /* meld_core_gatekeeper_node_hpp */
