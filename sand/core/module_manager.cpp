#include "sand/core/module_manager.hpp"

namespace sand {
  module_manager::module_manager(std::unique_ptr<source_worker> source,
                                 std::map<std::string, module_worker_ptr> modules) :
    source_{move(source)}, modules_{move(modules)}
  {
  }

  source_worker&
  module_manager::source()
  {
    return *source_;
  }

  std::map<std::string, module_worker_ptr>&
  module_manager::modules()
  {
    return modules_;
  }
}
