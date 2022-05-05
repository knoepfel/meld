#include "meld/core/module.hpp"
#include "meld/utilities/debug.hpp"
#include "test/data_levels.hpp"

#include <chrono>
#include <iostream>
#include <thread>
#include <typeinfo>

namespace meld::test {
  struct run_only_module {
    void
    process(run const& r, concurrency::unlimited) const
    {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1s);
      debug("Processing ", r.name(), ' ', r, " in run-only module.");
    }
  };

  MELD_REGISTER_MODULE(run_only_module, run)
}
