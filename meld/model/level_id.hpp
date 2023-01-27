#ifndef meld_model_level_id_hpp
#define meld_model_level_id_hpp

#include "meld/model/fwd.hpp"

#include "fmt/format.h"
#include "fmt/ranges.h"

#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace meld {
  class level_id : public std::enable_shared_from_this<level_id> {
  public:
    using const_ptr = std::shared_ptr<level_id const>;
    static level_id const& base();
    static const_ptr base_ptr();

    using hash_type = std::size_t;
    const_ptr make_child(std::size_t new_level_number, std::string const& level_name) const;
    std::string const& level_name() const noexcept;
    std::size_t depth() const noexcept;
    const_ptr parent(std::string const& level_name) const;
    const_ptr parent(std::size_t request_depth = -1ull) const;
    bool has_parent() const noexcept;
    std::size_t back() const;
    std::size_t hash() const noexcept;
    std::size_t level_hash() const noexcept;
    bool operator==(level_id const& other) const;
    bool operator<(level_id const& other) const;

    std::string to_string() const;
    std::string to_string_this_level() const;

    friend struct fmt::formatter<level_id>;
    friend std::ostream& operator<<(std::ostream& os, level_id const& id);

  private:
    level_id();
    explicit level_id(const_ptr parent, std::size_t i, std::string const& level_name);
    const_ptr parent_{nullptr};
    std::size_t number_{-1ull};
    std::string level_name_{"job"};
    std::size_t level_hash_;
    std::size_t depth_{};
    hash_type hash_{0};
  };

  using level_id_ptr = level_id::const_ptr;
  level_id_ptr id_for(char const* str);
  level_id_ptr id_for(std::vector<std::size_t> nums);
  level_id_ptr operator"" _id(char const* str, std::size_t);
  std::ostream& operator<<(std::ostream& os, level_id const& id);
}

namespace std {
  template <>
  struct hash<meld::level_id> {
    std::size_t operator()(meld::level_id const& id) const noexcept { return id.hash(); }
  };
}

#endif /* meld_model_level_id_hpp */
