#ifndef meld_core_transition_hpp
#define meld_core_transition_hpp

#include "oneapi/tbb/concurrent_hash_map.h"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace meld {
  using id_t = std::vector<std::size_t>;

  id_t id_for(char const* str);
  id_t operator"" _id(char const* str, std::size_t);

  bool has_parent(id_t const& id);
  id_t parent(id_t id);
  enum class stage { setup, flush, process };

  std::size_t hash_id(id_t const& id);

  struct IDHasher {
    std::size_t
    hash(meld::id_t const& id) const
    {
      return hash_id(id);
    }

    bool
    equal(meld::id_t const& a, meld::id_t const& b) const
    {
      return a == b;
    }
  };

  class level_counter {
  public:
    void record_parent(id_t const& id);
    std::optional<std::size_t> value_if_present(id_t const& id);
    std::size_t value(id_t const& id);
    id_t value_as_id(id_t id);

    void print() const;

  private:
    tbb::concurrent_hash_map<meld::id_t, unsigned, IDHasher> counter_;
    using accessor = decltype(counter_)::accessor;
  };

  // std::string is the name of the node type
  using transition_type = std::pair<std::string, stage>;
  using transition = std::pair<id_t, stage>;
  using transitions = std::vector<transition>;

  inline std::size_t
  hash_id_of_parent(transition const&)
  {
    return 0;
  }
  transitions transitions_between(id_t begin, id_t end, level_counter& counter);
  transitions transitions_for(std::vector<id_t> const& ids);

  std::string to_string(stage);
  std::ostream& operator<<(std::ostream& os, id_t const& id);
  std::ostream& operator<<(std::ostream& os, transition const& t);
}

#endif /* meld_core_transition_hpp */
