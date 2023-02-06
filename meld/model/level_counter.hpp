#ifndef meld_model_level_counter_hpp
#define meld_model_level_counter_hpp

#include "meld/model/fwd.hpp"
#include "meld/model/level_id.hpp"

#include "oneapi/tbb/concurrent_hash_map.h"

#include <cstddef>
#include <map>

namespace meld {
  class flush_counts {
  public:
    explicit flush_counts(std::string_view level_name);
    flush_counts(std::string_view level_name, std::map<std::string_view, std::size_t> child_counts);

    std::string_view level_name() const noexcept { return level_name_; }

    auto begin() const { return child_counts_.begin(); }
    auto end() const { return child_counts_.end(); }
    bool empty() const { return child_counts_.empty(); }
    auto size() const { return child_counts_.size(); }

    std::size_t count_for(std::string_view level_name) const
    {
      return child_counts_.at(level_name);
    }

  private:
    std::string_view level_name_;
    std::map<std::string_view, std::size_t> child_counts_{};
  };

  class level_counter {
  public:
    level_counter(level_counter* parent = nullptr, std::string_view level_name = "(root)");
    ~level_counter();

    level_counter make_child(std::string_view level_name);
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
    std::string_view level_name_;
    std::map<std::string_view, std::size_t> child_counts_{};
  };
}

#endif /* meld_model_level_counter_hpp */
