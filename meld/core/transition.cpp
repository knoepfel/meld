#include "meld/core/transition.hpp"

#include "boost/algorithm/string.hpp"

#include <algorithm>
#include <iostream>
#include <numeric>

namespace meld {
  bool
  has_parent(id_t const& id)
  {
    return not empty(id);
  }

  id_t
  parent(id_t id)
  {
    if (not has_parent(id))
      throw std::runtime_error("Empty ID does not have a parent.");
    id.pop_back();
    return id;
  }

  transitions
  transitions_between(id_t from, id_t const to, level_counter& counter)
  {
    if (from == to) {
      return {};
    }

    // std::size_t const to_size = size(to);
    auto const from_begin = begin(from);
    auto const from_end = end(from);
    auto const to_end = end(to);
    auto [from_it, to_it] = std::mismatch(from_begin, from_end, begin(to), to_end);

    // For an ID of the form [a, b, c, ..., k, l], the parent includes
    // all elements but the last one, to wit: [a, b, c, ..., k].
    auto const common_stages = distance(from_begin, from_it);

    transitions result;
    // 'process' stages to call

    for (auto i = size(from); i > common_stages; --i) {
      counter.record_parent(from);
      result.emplace_back(counter.value_as_id(from), stage::flush);
      result.emplace_back(from, stage::process);
      from.pop_back();
    }

    // 'setup' stages to call
    for (; to_it != to_end; ++to_it) {
      from.push_back(*to_it);
      result.emplace_back(from, stage::setup);
    }

    return result;
  }

  transitions
  transitions_for(std::vector<id_t> const& ids)
  {
    level_counter counter;

    // Equivalent of begin job/end job
    if (empty(ids)) {
      auto const id = ""_id;
      return {{id, stage::setup}, {counter.value_as_id(id), stage::flush}, {id, stage::process}};
    }

    auto from = begin(ids);
    // Account for initial transition to 'from' (including begin 'job' equivalent) ...
    transitions result{{""_id, stage::setup}};
    auto first_trs = transitions_between({}, *from, counter);
    result.insert(end(result), begin(first_trs), end(first_trs));

    std::map<id_t, unsigned> flush_numbers;
    for (auto to = next(from), e = end(ids); to != e; ++from, ++to) {
      auto trs = transitions_between(*from, *to, counter);
      result.insert(end(result), begin(trs), end(trs));
    }
    // ... and final transition to '{}', including end 'job' equivalent
    auto last_trs = transitions_between(*from, {}, counter);
    result.insert(end(result), begin(last_trs), end(last_trs));
    result.emplace_back(counter.value_as_id(""_id), stage::flush);
    result.emplace_back(""_id, stage::process);
    return result;
  }

  id_t operator"" _id(char const* c_str, std::size_t)
  {
    std::vector<std::string> strs;
    split(strs, c_str, boost::is_any_of(":"));

    strs.erase(std::remove_if(begin(strs), end(strs), [](auto& str) { return empty(str); }),
               end(strs));

    id_t result;
    std::transform(begin(strs), end(strs), std::back_inserter(result), [](auto const& str) {
      return std::stoull(str);
    });
    return result;
  }

  std::string
  to_string(stage const s)
  {
    switch (s) {
    case stage::setup:
      return "setup";
    case stage::process:
      return "process";
    case stage::flush:
      return "flush";
    }
  }

  std::ostream&
  operator<<(std::ostream& os, id_t const& id)
  {
    if (empty(id))
      return os << "[]";
    os << '[' << id.front();
    for (auto b = begin(id) + 1, e = end(id); b != e; ++b) {
      os << ", " << *b;
    }
    os << ']';
    return os;
  }

  std::ostream&
  operator<<(std::ostream& os, transition const& t)
  {
    return os << "ID: " << t.first << " Stage: " << to_string(t.second);
  }

  std::size_t
  hash_id(meld::id_t const& id)
  {
    // Pilfered from https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector#comment126511630_27216842
    return std::accumulate(begin(id), end(id), size(id), [](std::size_t h, std::size_t f) {
      return h ^= f + 0x9e3779b9 + (h << 6) + (h >> 2);
    });
  }

  void
  level_counter::record_parent(id_t const& id)
  {
    if (empty(id)) {
      // No parent to record
      return;
    }
    accessor a;
    if (counter_.insert(a, parent(id))) {
      a->second = 1;
    }
    else {
      ++a->second;
    }
  }

  std::optional<size_t>
  level_counter::value_if_present(id_t const& id)
  {
    if (accessor a; counter_.find(a, id)) {
      return a->second;
    }
    return std::nullopt;
  }

  std::size_t
  level_counter::value(id_t const& id)
  {
    if (accessor a; counter_.find(a, id)) {
      return a->second;
    }
    return 0;
  }

  id_t
  level_counter::value_as_id(id_t id)
  {
    id.push_back(value(id));
    return id;
  }

  void
  level_counter::print() const
  {
    for (auto const& [id, count] : counter_) {
      std::cout << id << " (" << count << ")\n";
    }
  }
}
