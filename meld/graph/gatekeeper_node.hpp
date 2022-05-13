#ifndef meld_graph_gatekeeper_node_hpp
#define meld_graph_gatekeeper_node_hpp

#include "meld/graph/node.hpp"
#include "meld/utilities/sized_tuple.hpp"
#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/flow_graph.h"

#include <tuple>
#include <utility>

namespace meld {
  namespace detail {
    template <std::size_t N>
    using msg_tuple = sized_tuple<transition_message, N>;
  }

  class gatekeeper_node :
    public tbb::flow::composite_node<detail::msg_tuple<2>, detail::msg_tuple<1>> {
  public:
    explicit gatekeeper_node(tbb::flow::graph& g);

  private:
    using base_t = tbb::flow::composite_node<detail::msg_tuple<2>, detail::msg_tuple<1>>;
    using indexer_node = tbb::flow::indexer_node<transition_message, transition_message>;
    using msg_t = indexer_node::output_type;

    bool is_ready(level_id const& id) const;
    bool is_initialized(level_id const& id) const;
    bool is_flush(level_id const& id);

    using multiplexer_node = tbb::flow::multifunction_node<msg_t, detail::msg_tuple<2>>;
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

#endif /* meld_graph_gatekeeper_node_hpp */
