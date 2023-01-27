#include "meld/model/level_counter.hpp"
#include "meld/model/level_id.hpp"

#include <iostream>

namespace meld {
  void level_counter::record_parent(level_id const& id)
  {
    if (not id.has_parent()) {
      // No parent to record
      return;
    }
    accessor a;
    if (counter_.insert(a, id.parent()->hash())) {
      a->second = 1;
    }
    else {
      ++a->second;
    }
  }

  std::size_t level_counter::value(level_id const& id) const
  {
    if (accessor a; counter_.find(a, id.hash())) {
      return a->second;
    }
    return 0;
  }
}
