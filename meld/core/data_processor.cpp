#include "meld/core/data_processor.hpp"
#include "meld/core/module_owner.hpp"
#include "meld/core/module_worker.hpp"
#include "meld/core/node.hpp"
#include "meld/core/source_owner.hpp"
#include "meld/core/source_worker.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <iostream>
#include <vector>

using namespace tbb;

namespace meld {

  data_processor::data_processor(module_manager* modules) :
    modules_{modules},
    source_node_{graph_, [this](flow_control& fc) { return pull_next(fc); }},
    process_{
      graph_, flow::unlimited, [this](transition_message const& msg) { return process(msg); }}
  {
    make_edge(source_node_, input_port<0>(gatekeeper_));
    make_edge(gatekeeper_, process_);
    make_edge(process_, input_port<1>(gatekeeper_));

    for (auto& [name, worker] : modules_->modules()) {
      for (auto const& transition_type : worker->supported_transitions()) {
        auto it = transition_graphs_.find(transition_type);
        if (it == cend(transition_graphs_)) {
          std::cout << "Creating graph for: " << transition_type.first << ' '
                    << to_string(transition_type.second) << '\n';
          it = transition_graphs_
                 .emplace(std::piecewise_construct,
                          std::forward_as_tuple(transition_type),
                          std::forward_as_tuple(transition_type.second))
                 .first;
        }
        it->second.add_node(name, *worker);
      }
    }

    for (auto& pr : transition_graphs_) {
      pr.second.calculate_edges();
    }
  }

  void
  data_processor::run_to_completion()
  {
    source_node_.activate();
    graph_.wait_for_all();
  }

  transition_message
  data_processor::pull_next(flow_control& fc)
  {
    if (queued_messages_.empty()) {
      auto data = modules_->source().next();
      for (auto const& d : data) {
        queued_messages_.push(d);
      }
    }

    if (transition_message msg; queued_messages_.try_pop(msg)) {
      return msg;
    }

    fc.stop();
    return {};
  }

  transition_message
  data_processor::process(transition_message const& msg)
  {
    auto const& [tr, node_ptr] = msg;
    auto const ttype = ttype_for(msg);
    if (auto it = transition_graphs_.find(ttype); it != cend(transition_graphs_)) {
      it->second.process(node_ptr);
    }
    return msg;
  }
}
