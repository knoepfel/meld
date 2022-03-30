#include "sand/core/data_levels.hpp"
#include "sand/core/module.hpp"

#include <iostream>

namespace sand::test {
  class user_module {
  public:
    void
    process(run&)
    {
      std::cout << "Calling into user module.\n";
    }
  };
  using module_to_use = module<user_module, run>;
}
