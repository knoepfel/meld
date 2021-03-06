#ifndef meld_graph_transition_hpp
#define meld_graph_transition_hpp

#include "oneapi/tbb/concurrent_hash_map.h"

#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace meld {
  class level_id;
  class level_counter;
  enum class stage { setup, flush, process };

  // std::string is the name of the data_node type
  using transition_type = std::pair<std::string, stage>;
  using transition = std::pair<level_id, stage>;
  using transitions = std::vector<transition>;

  class level_id {
  public:
    level_id();
    explicit level_id(std::initializer_list<std::size_t> numbers);
    explicit level_id(std::vector<std::size_t> numbers);
    level_id make_child(std::size_t new_level_number) const;
    level_id parent() const;
    bool has_parent() const noexcept;
    std::size_t back() const;
    std::size_t hash() const noexcept;
    bool operator==(level_id const& other) const;
    bool operator<(level_id const& other) const;

    friend transitions transitions_between(level_id from, level_id const& to, level_counter& c);
    friend std::ostream& operator<<(std::ostream& os, level_id const& id);

  private:
    std::vector<std::size_t> id_{};
    std::size_t hash_;
  };

  level_id id_for(char const* str);
  level_id operator"" _id(char const* str, std::size_t);

  struct IDHasher {
    std::size_t
    hash(level_id const& id) const noexcept
    {
      return id.hash();
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
    std::size_t value(level_id const& id) const;
    level_id value_as_id(level_id const& id) const;

    void print() const;

  private:
    tbb::concurrent_hash_map<level_id, unsigned, IDHasher> counter_;
    using accessor = decltype(counter_)::accessor;
  };

  transitions transitions_between(level_id begin, level_id const& end, level_counter& counter);
  transitions transitions_for(std::vector<level_id> const& ids);

  std::string to_string(stage);
  std::ostream& operator<<(std::ostream& os, level_id const& id);
  std::ostream& operator<<(std::ostream& os, transition const& t);
}

#endif /* meld_graph_transition_hpp */
