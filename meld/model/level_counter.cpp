#include "meld/model/level_counter.hpp"
#include "meld/model/level_id.hpp"

#include <iostream>

namespace meld {

  flush_counts::flush_counts(std::string const& level_name,
                             std::map<std::string, std::size_t> const& child_counts) :
    level_name_{level_name}, child_counts_{child_counts}
  {
  }

  level_counter::level_counter(level_counter* parent, std::string level_name) :
    parent_{parent}, level_name_{move(level_name)}
  {
  }

  level_counter::~level_counter()
  {
    if (parent_) {
      parent_->adjust(*this);
    }
  }

  level_counter level_counter::make_child(std::string const& level_name)
  {
    return {this, level_name};
  }

  void level_counter::adjust(level_counter& child)
  {
    ++child_counts_[child.level_name_];
    for (auto const& [nested_level_name, count] : child.child_counts_) {
      child_counts_[nested_level_name] += count;
    }
  }

}
