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
      std::cout << "Setting up run " << r << " in multiple-levels module.\n";
      setup_runs.push_back(r.id());
    }

    void
    process(subrun const& sr)
    {
      std::cout << "Processing subrun " << sr << " in multiple-levels module.\n";
      processed_subruns.push_back(sr.id());
    }

    void
    process(run const& r)
    {
      std::cout << "Processing run " << r << " in multiple-levels module.\n";
      processed_runs.push_back(r.id());
    }

    std::vector<id_t> setup_runs;
    std::vector<id_t> processed_runs;
    std::vector<id_t> processed_subruns;
  };
}

#endif /* test_test_module_multiple_levels_hpp */
