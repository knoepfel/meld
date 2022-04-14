#include "sand/core/transition.hpp"

#include "boost/algorithm/string.hpp"

#include <algorithm>
#include <iostream>

namespace sand {
  transitions
  transitions_between(id_t from, id_t const to)
  {
    if (from == to) {
      return {};
    }

    auto const from_end = end(from);
    auto const to_end = end(to);
    auto [from_it, to_it] = std::mismatch(begin(from), from_end, begin(to), to_end);

    transitions result;
    // 'process' stages to call
    for (auto process_stages = std::distance(from_it, from_end); process_stages > 0;
         --process_stages) {
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
    }
  }

  std::ostream&
  operator<<(std::ostream& os, id_t const& id)
  {
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

}
