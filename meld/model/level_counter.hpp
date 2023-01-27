#ifndef meld_model_level_counter_hpp
#define meld_model_level_counter_hpp

#include "meld/model/fwd.hpp"
#include "meld/model/level_id.hpp"

#include "oneapi/tbb/concurrent_hash_map.h"

#include <cstddef>
#include <map>

namespace meld {
  class level_counter {
  public:
    void record_parent(level_id const& id);
    std::size_t value(level_id const& id) const;

  private:
    tbb::concurrent_hash_map<level_id::hash_type, unsigned> counter_;
    using accessor = decltype(counter_)::accessor;
  };

  class flush_counts {
  public:
    flush_counts(std::string const& level_name,
                 std::map<std::string, std::size_t> const& child_counts) :
      level_name_{level_name}, child_counts_{child_counts}
    {
    }

    std::string const& level_name() const { return level_name_; }

    auto begin() const { return child_counts_.begin(); }
    auto end() const { return child_counts_.end(); }
    bool empty() const { return child_counts_.empty(); }
    auto size() const { return child_counts_.size(); }

    std::size_t count_for(std::string const& level_name) const
    {
      return child_counts_.at(level_name);
    }

  private:
    std::string level_name_;
    std::map<std::string, std::size_t> child_counts_{};
  };

  class level_counter_v2 {
  public:
    explicit level_counter_v2(level_counter_v2* parent = nullptr,
                              std::string level_name = "(root)") :
      parent_{parent}, level_name_{move(level_name)}
    {
    }

    level_counter_v2 make_child(std::string const& level_name);

    flush_counts result() const { return {level_name_, child_counts_}; }

    ~level_counter_v2()
    {
      if (parent_) {
        parent_->adjust(*this);
      }
    }

  private:
    void adjust(level_counter_v2& child);

    level_counter_v2* parent_;
    std::string level_name_;
    std::map<std::string, std::size_t> child_counts_{};
  };

  inline level_counter_v2 level_counter_v2::make_child(std::string const& level_name)
  {
    return level_counter_v2{this, level_name};
  }

  inline void level_counter_v2::adjust(level_counter_v2& child)
  {
    ++child_counts_[child.level_name_];
    for (auto const& [nested_level_name, count] : child.child_counts_) {
      child_counts_[nested_level_name] += count;
    }
  }

}

#endif /* meld_model_level_counter_hpp */
