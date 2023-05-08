#include "meld/model/level_counter.hpp"

namespace meld {

  flush_counts::flush_counts(std::string level_name) : level_name_{std::move(level_name)} {}

  flush_counts::flush_counts(std::string level_name,
                             std::map<std::string, std::size_t> child_counts) :
    level_name_{std::move(level_name)}, child_counts_{std::move(child_counts)}
  {
  }

  level_counter::level_counter(level_counter* parent, std::string level_name) :
    parent_{parent}, level_name_{std::move(level_name)}
  {
  }

  level_counter::~level_counter()
  {
    if (parent_) {
      parent_->adjust(*this);
    }
  }

  level_counter level_counter::make_child(std::string level_name)
  {
    return {this, std::move(level_name)};
  }

  void level_counter::adjust(level_counter& child)
  {
    auto it = child_counts_.find(child.level_name_);
    if (it == cend(child_counts_)) {
      it = child_counts_.try_emplace(child.level_name_, 0).first;
    }
    ++it->second;
    for (auto const& [nested_level_name, count] : child.child_counts_) {
      child_counts_[nested_level_name] += count;
    }
  }

}
