#include "meld/model/level_hierarchy.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include <list>
#include <ostream>

namespace {
  std::string const unnamed{"(unnamed)"};
  std::string const& maybe_name(std::string const& name) { return empty(name) ? unnamed : name; }
  std::string indent(std::size_t num)
  {
    std::string prefix;
    while (num-- != 0) {
      prefix.append("  ");
    }
    return prefix;
  }
}

namespace meld {

  void level_hierarchy::update(level_id_ptr const& id)
  {
    if (!levels_.contains(id->level_hash())) {
      auto const parent_hash = id->has_parent() ? id->parent()->level_hash() : -1ull;
      levels_[id->level_hash()] = {id->level_name(), parent_hash, id->depth()};
    }

    level_counter* parent_counter = nullptr;
    if (auto parent = id->parent()) {
      parent_counter = counters_.at(parent->hash()).get();
    }
    counters_[id->hash()] = std::make_shared<level_counter>(parent_counter, id->level_name());
  }

  flush_counts level_hierarchy::complete(level_id_ptr const& id)
  {
    ++levels_.at(id->level_hash()).count;
    auto counter = counters_.extract(id->hash());
    return counter.mapped()->result();
  }

  std::size_t level_hierarchy::count_for(std::string const& level_name) const
  {
    auto it = find_if(begin(levels_), end(levels_), [&level_name](auto const& level) {
      return level.second.name == level_name;
    });
    return it != cend(levels_) ? it->second.count : 0;
  }

  void level_hierarchy::print() const
  {
    for (auto const& str : graph_layout()) {
      spdlog::info(str);
    }
  }

  std::vector<std::string> level_hierarchy::graph_layout() const
  {
    if (empty(levels_)) {
      return {};
    }

    auto const b = levels_.begin();
    std::list<std::pair<std::size_t, level_entry>> ordered_levels{*b};
    for (auto it = next(b), e = levels_.end(); it != e; ++it) {
      auto parent_it =
        find_if(begin(ordered_levels), end(ordered_levels), [&it](auto const& ordered_level) {
          return it->second.parent_hash == ordered_level.first;
        });

      auto position = parent_it != end(ordered_levels) ? next(parent_it) : begin(ordered_levels);
      ordered_levels.insert(position, *it);
    }

    std::vector<std::string> result;
    result.reserve(size(ordered_levels));
    for (auto const& [_, entry] : ordered_levels) {
      result.push_back(
        fmt::format("{}{}: {}", indent(entry.depth), maybe_name(entry.name), entry.count));
    }
    return result;
  }
}
