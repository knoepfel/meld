#ifndef test_test_module_multiple_levels_hpp
#define test_test_module_multiple_levels_hpp

#include "sand/core/module.hpp"
#include "test/data_levels.hpp"

#include <iostream>
#include <vector>

namespace sand::test {
  struct multiple_levels {
    void
    setup(run const& r)
    {
      processed_transitions.emplace_back(r.id(), stage::setup);
    }

    void
    process(subrun const& sr)
    {
      processed_transitions.emplace_back(sr.id(), stage::process);
    }

    void
    process(run const& r)
    {
      processed_transitions.emplace_back(r.id(), stage::process);
    }

    transitions processed_transitions;
  };
}

#endif /* test_test_module_multiple_levels_hpp */
