#ifndef meld_core_data_processor_hpp
#define meld_core_data_processor_hpp

#include "meld/core/gatekeeper_node.hpp"
#include "meld/core/module_manager.hpp"
#include "meld/core/module_owner.hpp"
#include "meld/core/module_worker.hpp"
#include "meld/core/node.hpp"
#include "meld/core/source_owner.hpp"
#include "meld/core/source_worker.hpp"
#include "meld/core/transition_graph.hpp"

#include "oneapi/tbb/concurrent_queue.h"
#include "oneapi/tbb/flow_graph.h"

#include <iostream>
#include <vector>

namespace meld {
  class data_processor {
  public:
    explicit data_processor(module_manager* modules);
    void run_to_completion();

  private:
    transition_message pull_next(tbb::flow_control& fc);
    transition_message process(transition_message const& msg);

    module_manager* modules_;
    tbb::flow::graph graph_{};
    tbb::flow::input_node<transition_message> source_node_;
    tbb::concurrent_queue<transition_message> queued_messages_;
    gatekeeper_node gatekeeper_{graph_};
    tbb::flow::function_node<transition_message, transition_message> process_;
    std::map<transition_type, transition_graph> transition_graphs_;
  };
}

#endif /* meld_core_data_processor_hpp */
