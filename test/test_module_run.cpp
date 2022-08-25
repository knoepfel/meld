#include "meld/core/module.hpp"
#include "meld/utilities/debug.hpp"
#include "meld/utilities/sleep_for.hpp"
#include "test/data_levels.hpp"

namespace meld::test {
  struct run_only_module {
    void
    process(run const& r, concurrency::unlimited) const
    {
      sleep_for(1s);
      debug("Processing ", r.name(), ' ', r, " in run-only module.");
    }
  };

  MELD_REGISTER_MODULE(run_only_module, run)
}
