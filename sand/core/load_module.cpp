#include "sand/core/load_module.hpp"
#include "sand/core/test_module.hpp"

#include "boost/dll/import.hpp"

namespace sand {
  std::unique_ptr<module_worker>
  load_module()
  {
    return std::make_unique<test::module_to_use>();
  }
}
