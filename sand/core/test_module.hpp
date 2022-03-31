#include "sand/core/data_levels.hpp"
#include "sand/core/module.hpp"

#include <iostream>

namespace sand::test {
  class user_module {
  public:
    void
    process(run const&) const
    {
      std::cout << "Processing run in user module.\n";
    }

    void
    process(subrun const&) const
    {
      std::cout << "Processing subrun in user module.\n";
    }
  };
  using module_to_use = module<user_module, run>;
}
