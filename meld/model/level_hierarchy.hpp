#ifndef meld_model_level_hierarchy_hpp
#define meld_model_level_hierarchy_hpp

#include "meld/model/fwd.hpp"
#include "meld/model/level_counter.hpp"

#include <concepts>
#include <map>
#include <set>
#include <utility>
#include <vector>

namespace meld {

  class level_hierarchy {
  public:
    void update(level_id_ptr id);
    flush_counts complete(level_id_ptr id);

    std::size_t count_for(std::string const& level_name) const;

    void print() const;

  private:
    std::string graph_layout() const;

    using hash_name_pair = std::pair<std::string, std::size_t>;
    using hash_name_pairs = std::vector<hash_name_pair>;
    std::string pretty_recurse(std::map<std::string, hash_name_pairs> const& tree,
                               std::string const& parent_name,
                               std::string indent = {}) const;

    struct level_entry {
      std::string name;
      std::size_t parent_hash;
      std::size_t count;
    };

    std::map<std::size_t, level_entry> levels_;
    std::map<level_id::hash_type, std::shared_ptr<level_counter>> counters_;
  };

}

#endif /* meld_model_level_hierarchy_hpp */
