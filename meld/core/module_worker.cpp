#include "meld/core/module_worker.hpp"

// FIXME: Should move this dependency farther up and not at the module level.
#include "oneapi/tbb/flow_graph.h"

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

  std::size_t
  module_worker::concurrency(transition_type const& tt) const
  {
    return do_concurrency(tt);
  }

  void
  module_worker::process(stage const s, node& data)
  {
    do_process(s, data);
  }

  std::vector<std::string>
  module_worker::dependencies() const
  {
    return required_dependencies();
  }
}
