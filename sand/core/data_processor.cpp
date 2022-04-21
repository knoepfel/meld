#include "sand/core/data_processor.hpp"
#include "sand/core/module_owner.hpp"
#include "sand/core/module_worker.hpp"
#include "sand/core/node.hpp"
#include "sand/core/source_owner.hpp"
#include "sand/core/source_worker.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <iostream>
#include <vector>

namespace sand {

  data_processor::data_processor(module_manager* modules) :
    modules_{modules},
    source_node_{graph_, [this](tbb::flow_control& fc) { return pull_next(fc); }},
    engine_node_{graph_, tbb::flow::serial, [this](auto& messages) { process(messages); }}
  {
    make_edge(source_node_, engine_node_);
    for (auto& [name, worker] : modules_->modules()) {
      for (auto const& transition_type : worker->supported_transitions()) {
        auto it = transition_graphs_.find(transition_type);
        if (it == cend(transition_graphs_)) {
          std::cout << "Creaing graph for: " << transition_type.first << ' '
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

  transition_messages
  data_processor::pull_next(tbb::flow_control& fc)
  {
    auto data = modules_->source().next();
    if (empty(data)) {
      fc.stop();
      return {};
    }
    return data;
  }

  void
  data_processor::process(transition_messages messages)
  {
    for (auto& [transition_type, n] : messages) {
      auto it = transition_graphs_.find(transition_type);
      if (it == end(transition_graphs_)) {
        // Transition not supported for this job; will skip
        continue;
      }
      it->second.process(n);
    }
  }
}
