#ifndef sldkfjslkdjflskdjfds
#define sldkfjslkdjflskdjfds

#include "meld/core/transition.hpp"
#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/flow_graph.h"

namespace meld {
  namespace detail {
    using indexer_node = tbb::flow::indexer_node<transition, transition>;
    using msg_t = indexer_node::output_type;
    using gatekeeper_node_base = tbb::flow::composite_node<std::tuple<transition, transition>,
                                                           std::tuple<transition, transition>>;
  }

  class gatekeeper_node : public detail::gatekeeper_node_base {
  public:
    explicit gatekeeper_node(tbb::flow::graph& g);

  private:
    bool is_ready(id_t const& id) const;
    bool is_initialized(id_t const& id) const;
    bool is_flush(id_t const& id);

    using multiplexer_node =
      tbb::flow::multifunction_node<detail::msg_t, std::tuple<transition, transition, transition>>;
    using multiplexer_output_ports_type = multiplexer_node::output_ports_type;
    void multiplex(detail::msg_t const& msg, multiplexer_output_ports_type& outputs);

    tbb::concurrent_hash_map<meld::id_t, bool, IDHasher> setup_complete;
    using setup_accessor = decltype(setup_complete)::accessor;
    tbb::concurrent_hash_map<meld::id_t, unsigned, IDHasher> flush_values;
    using accessor = decltype(flush_values)::accessor;
    level_counter counter_;
    detail::indexer_node indexer_;
    multiplexer_node multiplexer_;
  };

}

#endif
