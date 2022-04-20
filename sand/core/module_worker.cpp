#include "sand/core/module_worker.hpp"

namespace sand {
  module_worker::~module_worker() = default;

  std::vector<transition_type>
  module_worker::supported_transitions() const
  {
    auto result = supported_setup_transitions();
    auto process_transitions = supported_process_transitions();
    result.insert(end(result), begin(process_transitions), end(process_transitions));
    return result;
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
