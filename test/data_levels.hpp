#ifndef test_data_levels_hpp
#define test_data_levels_hpp

#include "meld/core/data_level.hpp"

// Everything inside of here is considered "experiment-defined".

namespace meld {
  using run = data_level<"run">;
  using subrun = data_level<"subrun", run>;
}

#endif /* test_data_levels_hpp */
