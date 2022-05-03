#include "meld/core/gatekeeper_node.hpp"

using namespace tbb;

namespace meld {
  gatekeeper_node::gatekeeper_node(flow::graph& g) :
    base_t{g},
    indexer_{g},
    multiplexer_{g, flow::unlimited, [this](msg_t const& msg, auto& outputs) {
                   return multiplex(msg, outputs);
                 }}
  {
    make_edge(indexer_, multiplexer_);
    make_edge(output_port<1>(multiplexer_), input_port<0>(indexer_));

    set_external_ports(input_ports_type{input_port<0>(indexer_), input_port<1>(indexer_)},
                       output_ports_type{output_port<0>(multiplexer_)});
  }

  bool
  gatekeeper_node::is_ready(meld::level_id const& id) const
  {
    if (not id.has_parent())
      return true;
    return setup_complete.count(id.parent()) == 1ull;
  }

  bool
  gatekeeper_node::is_initialized(meld::level_id const& id) const
  {
    if (not id.has_parent())
      return true;
    return setup_complete.count(id) == 1ull;
  }

  bool
  gatekeeper_node::is_flush(meld::level_id const& id)
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
  gatekeeper_node::multiplex(msg_t const& msg, multiplexer_output_ports_type& outputs)
  {
    auto tr_msg = msg.cast_to<transition_message>();
    auto const& [id, stage] = tr_msg.first;
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

    auto& [process, retry] = outputs;
    switch (stage) {
    case stage::setup: {
      if (is_ready(id)) {
        process.try_put(tr_msg);
      }
      else {
        retry.try_put(tr_msg);
      }
      break;
    }
    case stage::process: {
      if (is_flush(id)) {
        process.try_put(tr_msg);
      }
      else {
        retry.try_put(tr_msg);
      }
      break;
    }
    case stage::flush: {
      flush_values.emplace(id.parent(), id.back());
    }
    }
  }
}
