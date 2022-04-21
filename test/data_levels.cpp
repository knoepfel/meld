#include "test/data_levels.hpp"

namespace meld {
  template class data_level<"run">;
  template class data_level<"subrun", run>;
}
