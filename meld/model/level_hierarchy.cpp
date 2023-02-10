#include "meld/model/level_hierarchy.hpp"

#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include <ostream>

namespace {
  std::string const unnamed{"(unnamed)"};
  std::string const& maybe_name(std::string const& name) { return empty(name) ? unnamed : name; }
}

namespace meld {

  void level_hierarchy::update(level_id_ptr const id)
  {
    if (!levels_.contains(id->level_hash())) {
      auto const parent_hash = id->has_parent() ? id->parent()->level_hash() : -1ull;
      levels_[id->level_hash()] = {id->level_name(), parent_hash};
    }

    level_counter* parent_counter = nullptr;
    if (auto parent = id->parent()) {
      auto it = counters_.find(parent->hash());
      assert(it != counters_.cend());
      parent_counter = it->second.get();
    }
    counters_[id->hash()] = std::make_shared<level_counter>(parent_counter, id->level_name());
  }

  flush_counts level_hierarchy::complete(level_id_ptr const id)
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

  void level_hierarchy::print() const { spdlog::info("{}", graph_layout()); }

  std::string level_hierarchy::pretty_recurse(std::map<std::string, hash_name_pairs> const& tree,
                                              std::string const& name,
                                              std::string indent) const
  {
    auto it = tree.find(name);
    if (it == cend(tree)) {
      return {};
    }

    std::string result;
    std::size_t const n = it->second.size();
    for (std::size_t i = 0; auto const& [child_name, child_hash] : it->second) {
      bool const at_end = ++i == n;
      auto child_prefix = !at_end ? indent + " ├ " : indent + " └ ";
      auto const& entry = levels_.at(child_hash);
      result += "\n" + indent + " │ ";
      result += fmt::format("\n{}{}: {}", child_prefix, maybe_name(child_name), entry.count);

      auto new_indent = indent;
      new_indent += at_end ? "   " : " │ ";
      result += pretty_recurse(tree, child_name, new_indent);
    }
    return result;
  }

  std::string level_hierarchy::graph_layout() const
  {
    if (empty(levels_)) {
      return {};
    }

    std::map<std::string, std::vector<hash_name_pair>> tree;
    for (auto const& [level_hash, level_entry] : levels_) {
      auto parent_hash = level_entry.parent_hash;
      if (parent_hash == -1ull) {
        continue;
      }
      auto const& parent_name = levels_.at(parent_hash).name;
      tree[parent_name].emplace_back(level_entry.name, level_hash);
    }

    auto const initial_indent = "  ";
    return fmt::format("\nProcessed levels:\n\n{}job{}\n",
                       initial_indent,
                       pretty_recurse(tree, "job", initial_indent));
  }
}
