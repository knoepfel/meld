#include "meld/model/transition.hpp"

#include "boost/algorithm/string.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <numeric>
#include <stdexcept>
#include <tuple>

namespace {
  std::size_t hash_nums(std::vector<std::size_t> const& nums) noexcept
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

  meld::level_id const base_id{};
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

  level_id const& level_id::base() { return base_id; }

  std::size_t level_id::depth() const noexcept { return id_.size(); }

  level_id level_id::make_child(std::size_t const new_level_number) const
  {
    auto numbers = id_;
    numbers.push_back(new_level_number);
    return level_id{move(numbers)};
  }

  bool level_id::has_parent() const noexcept
  {
    // Use std::vector::empty member function which is noexcept.
    return not id_.empty();
  }

  std::size_t level_id::back() const { return id_.back(); }
  std::size_t level_id::hash() const noexcept { return hash_; }

  bool level_id::operator==(level_id const& other) const { return id_ == other.id_; }
  bool level_id::operator<(level_id const& other) const { return id_ < other.id_; }

  level_id id_for(char const* c_str)
  {
    std::vector<std::string> strs;
    split(strs, c_str, boost::is_any_of(":"));

    strs.erase(std::remove_if(begin(strs), end(strs), [](auto& str) { return empty(str); }),
               end(strs));

    std::vector<std::size_t> numbers;
    transform(begin(strs), end(strs), back_inserter(numbers), [](auto const& str) {
      return std::stoull(str);
    });
    return level_id{move(numbers)};
  }

  level_id operator"" _id(char const* c_str, std::size_t) { return id_for(c_str); }

  level_id level_id::parent(std::size_t requested_depth) const
  {
    if (not has_parent())
      throw std::runtime_error("Empty ID does not have a parent.");

    if (requested_depth == -1ull) {
      requested_depth = depth() - 1;
    }
    auto id = id_;
    id.resize(requested_depth);
    return level_id{move(id)};
  }

  std::ostream& operator<<(std::ostream& os, level_id const& id)
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
}
