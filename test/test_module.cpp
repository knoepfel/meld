#include "sand/core/data_levels.hpp"
#include "sand/core/module.hpp"

#include <iostream>

namespace sand::test {
  class user_module {
  public:
    explicit user_module(boost::json::object const&) {}

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

  SAND_REGISTER_MODULE(user_module, run, subrun)
}
