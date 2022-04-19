#ifndef sand_core_data_processor_hpp
#define sand_core_data_processor_hpp

#include "sand/core/module_manager.hpp"
#include "sand/core/module_owner.hpp"
#include "sand/core/module_worker.hpp"
#include "sand/core/node.hpp"
#include "sand/core/source_owner.hpp"
#include "sand/core/source_worker.hpp"
#include "sand/core/transition_graph.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <iostream>
#include <vector>

namespace sand {

  class data_processor {
  public:
    explicit data_processor(module_manager* modules);
    void run_to_completion();

  private:
    transition_messages pull_next(tbb::flow_control& fc);
    void process(transition_messages messages);

    module_manager* modules_;
    tbb::flow::graph graph_{};
    tbb::flow::input_node<transition_messages> source_node_;
    std::map<transition_type, transition_graph> transition_graphs_;
    std::vector<tbb::flow::function_node<transition_messages>> engine_nodes_{};
  };
}

#endif /* sand_core_data_processor_hpp */
