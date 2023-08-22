#ifndef meld_model_level_counter_hpp
#define meld_model_level_counter_hpp

#include "meld/model/fwd.hpp"
#include "meld/model/level_id.hpp"

#include "oneapi/tbb/concurrent_hash_map.h"

#include <cstddef>
#include <map>
#include <optional>

namespace meld {
  class flush_counts {
  public:
    explicit flush_counts(std::string level_name);
    flush_counts(std::string level_name, std::map<std::string, std::size_t> child_counts);

    std::string_view level_name() const noexcept { return level_name_; }

    auto begin() const { return child_counts_.begin(); }
    auto end() const { return child_counts_.end(); }
    bool empty() const { return child_counts_.empty(); }
    auto size() const { return child_counts_.size(); }

    std::optional<std::size_t> count_for(std::string const& level_name) const
    {
      if (auto it = child_counts_.find(level_name); it != child_counts_.end()) {
        return it->second;
      }
      return std::nullopt;
    }

  private:
    std::string level_name_;
    std::map<std::string, std::size_t> child_counts_{};
  };

  using flush_counts_ptr = std::shared_ptr<flush_counts const>;

  class level_counter {
  public:
    level_counter(level_counter* parent = nullptr, std::string level_name = {});
    ~level_counter();

    level_counter make_child(std::string level_name);
    flush_counts result() const
    {
      if (empty(child_counts_)) {
        return flush_counts{level_name_};
      }
      return {level_name_, child_counts_};
    }

  private:
    void adjust(level_counter& child);

    level_counter* parent_;
    std::string level_name_;
    std::map<std::string, std::size_t> child_counts_{};
  };

  class flush_counters {
  public:
    void update(level_id_ptr const id);
    flush_counts extract(level_id_ptr const id);

  private:
    std::map<level_id::hash_type, std::shared_ptr<level_counter>> counters_;
  };
}

#endif /* meld_model_level_counter_hpp */
