#ifndef test_data_levels_hpp
#define test_data_levels_hpp

#include "sand/core/data_level.hpp"

// Everything inside of here is considered "experiment-defined".

namespace sand {
  using run = data_level<>;
  using subrun = data_level<run>;

  extern template class data_level<>;
  extern template class data_level<run>;
}

#endif /* test_data_levels_hpp */