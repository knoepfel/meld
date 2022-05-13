#ifndef meld_core_data_processor_hpp
#define meld_core_data_processor_hpp

#include "meld/core/module_manager.hpp"
#include "meld/core/module_owner.hpp"
#include "meld/core/source_owner.hpp"
#include "meld/core/source_worker.hpp"
#include "meld/graph/gatekeeper_node.hpp"
#include "meld/graph/module_worker.hpp"
#include "meld/graph/node.hpp"
#include "meld/graph/transition_graph.hpp"

#include "oneapi/tbb/concurrent_queue.h"
#include "oneapi/tbb/flow_graph.h"

#include <iostream>
#include <vector>

namespace meld {
  class data_processor {
    using process_switch =
      tbb::flow::multifunction_node<transition_message, std::tuple<transition_message>>;
    using process_output_ports_type = process_switch::output_ports_type;

  public:
    explicit data_processor(module_manager* modules);
    void run_to_completion();

  private:
    transition_message pull_next(tbb::flow_control& fc);
    void process(transition_message const& msg, process_output_ports_type& outputs);

    module_manager* modules_;
    tbb::flow::graph graph_{};
    tbb::flow::input_node<transition_message> source_node_;
    tbb::concurrent_queue<transition_message> queued_messages_;
    gatekeeper_node gatekeeper_{graph_};
    process_switch process_;
    std::map<transition_type, transition_graph> transition_graphs_;
  };
}

#endif /* meld_core_data_processor_hpp */
