#include "sand/core/data_levels.hpp"
#include "sand/core/module_owner.hpp"

#include <iostream>

namespace sand::test {
  class user_module {
  public:
    void
    process(run const& r) const
    {
      std::cout << "Processing run " << r.id() << " in user module.\n";
    }

    void
    process(subrun const& sr) const
    {
      std::cout << "Processing subrun " << sr.id() << " in user module.\n";
    }
  };
  using module_to_use = module_owner<user_module, run, subrun>;
}
