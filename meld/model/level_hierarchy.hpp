#ifndef meld_model_level_hierarchy_hpp
#define meld_model_level_hierarchy_hpp

#include "meld/model/fwd.hpp"
#include "meld/model/level_counter.hpp"

#include <concepts>
#include <iosfwd>
#include <set>
#include <utility>
#include <vector>

namespace meld {

  class level_hierarchy {
  public:
    void update(level_id_ptr const& id);
    flush_counts complete(level_id_ptr const& id);

    std::size_t count_for(std::string const& level_name) const;
    auto const& all_counts() const { return levels_; }

    void print() const;

  private:
    std::vector<std::string> graph_layout() const;

    struct level_entry {
      std::string name;
      std::size_t parent_hash;
      std::size_t depth;
      std::size_t count;
    };

    // FIXME: Doesn't support levels with the same names but different parents.
    std::map<std::size_t, level_entry> levels_;
    std::map<level_id::hash_type, std::shared_ptr<level_counter_v2>> counters_;
  };

}

#endif /* meld_model_level_hierarchy_hpp */
