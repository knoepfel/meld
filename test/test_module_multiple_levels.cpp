#include "sand/core/module.hpp"
#include "test/data_levels.hpp"

#include <iostream>

namespace sand::test {
  class multiple_levels {
  public:
    void
    process(run const& r) const
    {
      std::cout << "Processing run " << r << " in multiple-levels module.\n";
    }

    void
    process(subrun const& sr) const
    {
      std::cout << "Processing subrun " << sr << " in multiple-levels module.\n";
    }
  };

  SAND_REGISTER_MODULE(multiple_levels, run, subrun)
}
