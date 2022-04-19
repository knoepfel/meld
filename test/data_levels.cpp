#include "test/data_levels.hpp"

namespace sand {
  template class data_level<"run">;
  template class data_level<"subrun", run>;
}
