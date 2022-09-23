#include "meld/graph/transition.hpp"

#include "boost/algorithm/string.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <stdexcept>
#include <tuple>

namespace {
  std::size_t
  hash_nums(std::vector<std::size_t> const& nums) noexcept
  {
    // Pilfered from
    // https://stackoverflow.com/questions/20511347/a-good-hash-function-for-a-vector#comment126511630_27216842
    // The std::size() free function is not noexcept, so we use the
    // std::vector::size() member function.
    return std::accumulate(
      cbegin(nums), cend(nums), nums.size(), [](std::size_t h, std::size_t f) noexcept {
        return h ^= f + 0x9e3779b9 + (h << 6) + (h >> 2);
      });
  }
}

namespace meld {

  level_id::level_id() : hash_{hash_nums({})} {}
  level_id::level_id(std::initializer_list<std::size_t> numbers) :
    level_id{std::vector<std::size_t>{numbers}}
  {
  }
  level_id::level_id(std::vector<std::size_t> numbers) : id_{move(numbers)}, hash_{hash_nums(id_)}
  {
  }

  std::size_t
  level_id::depth() const noexcept
  {
    return id_.size();
  }

  level_id
  level_id::make_child(std::size_t const new_level_number) const
  {
    auto numbers = id_;
    numbers.push_back(new_level_number);
    return level_id{move(numbers)};
  }

  bool
  level_id::has_parent() const noexcept
  {
    // Use std::vector::empty member function which is noexcept.
    return not id_.empty();
  }

  std::size_t
  level_id::back() const
  {
    return id_.back();
  }

  std::size_t
  level_id::hash() const noexcept
  {
    return hash_;
  }

  bool
  level_id::operator==(level_id const& other) const
  {
    return id_ == other.id_;
  }

  bool
  level_id::operator<(level_id const& other) const
  {
    return id_ < other.id_;
  }

  level_id
  id_for(char const* c_str)
  {
    std::vector<std::string> strs;
    split(strs, c_str, boost::is_any_of(":"));

    strs.erase(std::remove_if(begin(strs), end(strs), [](auto& str) { return empty(str); }),
               end(strs));

    std::vector<std::size_t> numbers;
    std::transform(begin(strs), end(strs), back_inserter(numbers), [](auto const& str) {
      return std::stoull(str);
    });
    return level_id{move(numbers)};
  }

  level_id operator"" _id(char const* c_str, std::size_t) { return id_for(c_str); }

  level_id
  level_id::parent() const
  {
    if (not has_parent())
      throw std::runtime_error("Empty ID does not have a parent.");
    auto id = id_;
    id.pop_back();
    return level_id{move(id)};
  }

  transitions
  transitions_between(level_id from_id, level_id const& to_id, level_counter& counter)
  {
    if (from_id == to_id) {
      return {};
    }

    auto const& [from, to] = std::tie(from_id.id_, to_id.id_);
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
      counter.record_parent(from_id);
      result.emplace_back(counter.value_as_id(from_id), stage::flush);
      result.emplace_back(from_id, stage::process);
      from_id = from_id.parent();
    }

    // 'setup' stages to call
    for (; to_it != to_end; ++to_it) {
      from_id = from_id.make_child(*to_it);
      result.emplace_back(from_id, stage::setup);
    }

    return result;
  }

  transitions
  transitions_for(std::vector<level_id> const& ids)
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

    std::map<level_id, unsigned> flush_numbers;
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
  operator<<(std::ostream& os, level_id const& id)
  {
    if (not id.has_parent())
      return os << "[]";
    auto const& nums = id.id_;
    os << '[' << nums.front();
    for (auto b = begin(nums) + 1, e = end(nums); b != e; ++b) {
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

  void
  level_counter::record_parent(level_id const& id)
  {
    if (not id.has_parent()) {
      // No parent to record
      return;
    }
    accessor a;
    if (counter_.insert(a, id.parent())) {
      a->second = 1;
    }
    else {
      ++a->second;
    }
  }

  std::size_t
  level_counter::value(level_id const& id) const
  {
    if (accessor a; counter_.find(a, id)) {
      return a->second;
    }
    return 0;
  }

  level_id
  level_counter::value_as_id(level_id const& id) const
  {
    return id.make_child(value(id));
  }

  void
  level_counter::print() const
  {
    for (auto const& [id, count] : counter_) {
      std::cout << id << " (" << count << ")\n";
    }
  }
}
