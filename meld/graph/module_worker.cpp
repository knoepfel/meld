#include "meld/graph/module_worker.hpp"

#include <cassert>

namespace meld {
  module_worker::~module_worker() = default;

  std::vector<transition_type>
  module_worker::supported_transitions() const
  {
    auto result = supported_setup_transitions();
    auto process_transitions = supported_process_transitions();
    result.insert(end(result), begin(process_transitions), end(process_transitions));
    return result;
  }

  std::shared_ptr<module_node>
  module_worker::create_worker_node(tbb::flow::graph& g,
                                    transition_type const& tt,
                                    serializers& serialized_resources)
  {
    return do_create_worker_node(g, tt, serialized_resources);
  }

  std::vector<std::string>
  module_worker::dependencies() const
  {
    return required_dependencies();
  }
}
