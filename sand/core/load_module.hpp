#ifndef sand_core_load_module_hpp
#define sand_core_load_module_hpp

#include "sand/core/module_worker.hpp"

#include <memory>

namespace sand {
  std::unique_ptr<module_worker> load_module();
}

#endif
