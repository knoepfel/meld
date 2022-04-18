#ifndef sand_core_data_processor_hpp
#define sand_core_data_processor_hpp

#include "sand/core/module_manager.hpp"
#include "sand/core/module_owner.hpp"
#include "sand/core/module_worker.hpp"
#include "sand/core/node.hpp"
#include "sand/core/source_owner.hpp"
#include "sand/core/source_worker.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <vector>

namespace sand {

  class data_processor {
  public:
    explicit data_processor(module_manager* modules) : modules_{modules} {}

    void
    run_to_completion()
    {
      auto node = tbb::flow::function_node<transition_messages>{
        graph_, tbb::flow::serial, [this](auto& messages) { process(messages); }};
      auto& last_made = engine_nodes_.emplace_back(move(node));
      make_edge(source_node_, last_made);
      pull_next();
      graph_.wait_for_all();
    }

  private:
    void
    pull_next()
    {
      auto data = modules_->source().next();
      if (empty(data)) {
        return;
      }
      source_node_.try_put(data);
    }

    void
    process(transition_messages messages)
    {
      tbb::flow::graph g;
      // Probably will be using a dependency graph instead of a data-flow graph here.
      for (auto& [stage, n] : messages) {
        for (auto& pr : modules_->modules()) {
          pr.second->process(stage, *n);
        }
      }
      pull_next();
    }

    module_manager* modules_;
    tbb::flow::graph graph_{};
    tbb::flow::buffer_node<transition_messages> source_node_{graph_};
    std::vector<tbb::flow::function_node<transition_messages>> engine_nodes_{};
  };
}

#endif /* sand_core_data_processor_hpp */
