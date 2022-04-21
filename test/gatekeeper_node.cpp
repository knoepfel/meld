#include "test/gatekeeper_node.hpp"

namespace meld {
  gatekeeper_node::gatekeeper_node(tbb::flow::graph& g) :
    detail::gatekeeper_node_base{g},
    indexer_{g},
    multiplexer_{g, tbb::flow::unlimited, [this](detail::msg_t const& msg, auto& outputs) {
                   return multiplex(msg, outputs);
                 }}
  {
    make_edge(indexer_, multiplexer_);
    make_edge(output_port<2>(multiplexer_), input_port<0>(indexer_));

    set_external_ports(
      input_ports_type{input_port<0>(indexer_), input_port<1>(indexer_)},
      output_ports_type{output_port<0>(multiplexer_), output_port<1>(multiplexer_)});
  }

  bool
  gatekeeper_node::is_ready(meld::id_t const& id) const
  {
    if (not has_parent(id))
      return true;
    return setup_complete.count(parent(id)) == 1ull;
  }

  bool
  gatekeeper_node::is_initialized(meld::id_t const& id) const
  {
    if (not has_parent(id))
      return true;
    return setup_complete.count(id) == 1ull;
  }

  bool
  gatekeeper_node::is_flush(meld::id_t const& id)
  {
    if (not is_initialized(id)) {
      return false;
    }
    if (accessor a; flush_values.find(a, id)) {
      if (a->second == counter_.value(id)) {
        flush_values.erase(a);
        return true;
      }
    }
    return false;
  }

  void
  gatekeeper_node::multiplex(detail::msg_t const& msg, multiplexer_output_ports_type& outputs)
  {
    auto tr = msg.cast_to<transition>();
    auto const& [id, stage] = tr;
    if (msg.tag() == 1) {
      if (stage == stage::process) {
        counter_.record_parent(id);
        // FIXME: To save memory, we should erase the entry in the setup_complete hash map.
      }
      else if (stage == stage::setup) {
        setup_complete.emplace(id, true);
      }
      return;
    }

    auto& [initialize, process, retry] = outputs;
    switch (stage) {
    case stage::setup: {
      if (is_ready(id)) {
        initialize.try_put(tr);
      }
      else {
        retry.try_put(tr);
      }
      break;
    }
    case stage::process: {
      if (is_flush(id)) {
        process.try_put(tr);
      }
      else {
        retry.try_put(tr);
      }
      break;
    }
    case stage::flush: {
      flush_values.emplace(parent(id), id.back());
    }
    }
  }
}
