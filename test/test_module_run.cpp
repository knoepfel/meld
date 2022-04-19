#include "sand/core/module.hpp"
#include "test/data_levels.hpp"

#include <iostream>
#include <typeinfo>

namespace sand::test {
  class run_only_module {
  public:
    void
    process(run const& r) const
    {
      std::cout << "Processing " << r.name() << ' ' << r << " in run-only module.\n";
    }
  };

  SAND_REGISTER_MODULE(run_only_module, run)
}
