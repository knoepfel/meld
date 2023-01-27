#ifndef meld_model_level_counter_hpp
#define meld_model_level_counter_hpp

#include "meld/model/fwd.hpp"
#include "meld/model/transition.hpp"

#include "oneapi/tbb/concurrent_hash_map.h"

#include <cstddef>

namespace meld {
  class level_counter {
  public:
    void record_parent(level_id const& id);
    std::size_t value(level_id const& id) const;
    level_id value_as_id(level_id const& id) const;

    void print() const;

  private:
    tbb::concurrent_hash_map<level_id::hash_type, unsigned> counter_;
    using accessor = decltype(counter_)::accessor;
  };
}

#endif /* meld_model_level_counter_hpp */
