#include "meld/model/transition.hpp"
#include "meld/model/level_counter.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <tuple>

namespace meld {

  transitions transitions_between(level_id from_id, level_id const& to_id, level_counter& counter)
  {
    if (from_id == to_id) {
      return {};
    }

    auto const& [from, to] = tie(from_id.id_, to_id.id_);
    auto const from_begin = begin(from);
    auto const from_end = end(from);
    auto const to_end = end(to);
    auto [from_it, to_it] = mismatch(from_begin, from_end, begin(to), to_end);

    // For an ID of the form [a, b, c, ..., k, l], the parent includes
    // all elements but the last one, to wit: [a, b, c, ..., k].
    auto const common_stages = distance(from_begin, from_it);

    transitions result;
    // 'flush' stages to call

    for (auto i = size(from); i > static_cast<std::size_t>(common_stages); --i) {
      counter.record_parent(from_id);
      result.emplace_back(counter.value_as_id(from_id), stage::flush);
      from_id = from_id.parent();
    }

    // 'process' stages to call
    for (; to_it != to_end; ++to_it) {
      from_id = from_id.make_child(*to_it);
      result.emplace_back(from_id, stage::process);
    }

    return result;
  }

  transitions transitions_for(std::vector<level_id> const& ids)
  {
    level_counter counter;

    // Equivalent of begin job/end job
    if (empty(ids)) {
      auto const id = ""_id;
      return {{id, stage::process}, {counter.value_as_id(id), stage::flush}};
    }

    auto from = begin(ids);
    // Account for initial transition to 'from' (including begin 'job' equivalent) ...
    transitions result{{""_id, stage::process}};
    auto first_trs = transitions_between({}, *from, counter);
    result.insert(end(result), begin(first_trs), end(first_trs));

    std::map<level_id, unsigned> flush_numbers;
    for (auto to = next(from), e = end(ids); to != e; ++from, ++to) {
      auto trs = transitions_between(*from, *to, counter);
      result.insert(end(result), begin(trs), end(trs));
    }
    // ... and final transition to '{}', including end 'job' equivalent
    auto last_trs = transitions_between(*from, {}, counter);
    result.insert(end(result), begin(last_trs), end(last_trs));
    result.emplace_back(counter.value_as_id(""_id), stage::flush);
    return result;
  }

  std::string to_string(stage const s)
  {
    switch (s) {
    case stage::process:
      return "process";
    case stage::flush:
      return "flush";
    }
    return {};
  }

  std::ostream& operator<<(std::ostream& os, transition const& t)
  {
    return os << "ID: " << t.first << " Stage: " << to_string(t.second);
  }
}
