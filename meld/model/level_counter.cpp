#include "meld/model/level_counter.hpp"

namespace meld {

  flush_counts::flush_counts(std::string_view level_name) : level_name_{level_name} {}

  flush_counts::flush_counts(std::string_view level_name,
                             std::map<std::string_view, std::size_t> child_counts) :
    level_name_{level_name}, child_counts_{move(child_counts)}
  {
  }

  level_counter::level_counter(level_counter* parent, std::string_view level_name) :
    parent_{parent}, level_name_{move(level_name)}
  {
  }

  level_counter::~level_counter()
  {
    if (parent_) {
      parent_->adjust(*this);
    }
  }

  level_counter level_counter::make_child(std::string_view level_name)
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
