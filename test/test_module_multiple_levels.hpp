#ifndef test_test_module_multiple_levels_hpp
#define test_test_module_multiple_levels_hpp

#include "meld/core/module.hpp"
#include "test/data_levels.hpp"

#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

namespace meld::test {
  struct multiple_levels {
    void
    setup(run const& r, concurrency::serial)
    {
      std::lock_guard lock{m};
      processed_transitions.emplace_back(r.id(), stage::setup);
    }

    void
    process(subrun const& sr, concurrency::serial_for<"ROOT", "GENIE">)
    {
      std::lock_guard lock{m};
      processed_transitions.emplace_back(sr.id(), stage::process);
    }

    void
    process(run const& r, concurrency::unlimited)
    {
      using namespace std::chrono_literals;
      std::this_thread::sleep_for(1s);
      std::lock_guard lock{m};
      processed_transitions.emplace_back(r.id(), stage::process);
    }

    transitions processed_transitions;
    std::mutex m;
  };
}

#endif /* test_test_module_multiple_levels_hpp */
