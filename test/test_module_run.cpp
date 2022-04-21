#include "meld/core/module.hpp"
#include "test/data_levels.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <typeinfo>

namespace meld::test {
  class run_only_module {
  public:
    void
    process(run const& r) const
    {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1s);
      std::cout << "Processing " << r.name() << ' ' << r << " in run-only module.\n";
    }
  };

  MELD_REGISTER_MODULE(run_only_module, run)
}
