#ifndef meld_core_transition_hpp
#define meld_core_transition_hpp

#include "oneapi/tbb/concurrent_hash_map.h"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace meld {
  using level_id = std::vector<std::size_t>;

  level_id id_for(char const* str);
  level_id operator"" _id(char const* str, std::size_t);

  bool has_parent(level_id const& id);
  level_id parent(level_id id);
  enum class stage { setup, flush, process };

  std::size_t hash_id(level_id const& id);

  struct IDHasher {
    std::size_t
    hash(level_id const& id) const
    {
      return hash_id(id);
    }

    bool
    equal(level_id const& a, level_id const& b) const
    {
      return a == b;
    }
  };

  class level_counter {
  public:
    void record_parent(level_id const& id);
    std::optional<std::size_t> value_if_present(level_id const& id);
    std::size_t value(level_id const& id);
    level_id value_as_id(level_id id);

    void print() const;

  private:
    tbb::concurrent_hash_map<level_id, unsigned, IDHasher> counter_;
    using accessor = decltype(counter_)::accessor;
  };

  // std::string is the name of the node type
  using transition_type = std::pair<std::string, stage>;
  using transition = std::pair<level_id, stage>;
  using transitions = std::vector<transition>;

  transitions transitions_between(level_id begin, level_id const& end, level_counter& counter);
  transitions transitions_for(std::vector<level_id> const& ids);

  std::string to_string(stage);
  std::ostream& operator<<(std::ostream& os, level_id const& id);
  std::ostream& operator<<(std::ostream& os, transition const& t);
}

#endif /* meld_core_transition_hpp */
