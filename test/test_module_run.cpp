#include "sand/core/data_levels.hpp"
#include "sand/core/module.hpp"

#include <iostream>

namespace sand::test {
  class run_only_module {
  public:
    explicit run_only_module(boost::json::object const&) {}

    void
    process(run const& r) const
    {
      std::cout << "Processing run " << r << " in run-only module.\n";
    }
  };

  SAND_REGISTER_MODULE(run_only_module, run)
}
