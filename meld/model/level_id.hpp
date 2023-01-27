#ifndef meld_model_level_id_hpp
#define meld_model_level_id_hpp

#include "meld/model/fwd.hpp"

#include "fmt/format.h"
#include "fmt/ranges.h"

#include <cstddef>
#include <initializer_list>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

namespace meld {
  class level_id {
  public:
    level_id();
    static level_id const& base();

    using hash_type = std::size_t;
    explicit level_id(std::initializer_list<std::size_t> numbers);
    explicit level_id(std::vector<std::size_t> numbers);
    level_id make_child(std::size_t new_level_number) const;
    std::size_t depth() const noexcept;
    level_id parent(std::size_t request_depth = -1ull) const;
    bool has_parent() const noexcept;
    std::size_t back() const;
    std::size_t hash() const noexcept;
    bool operator==(level_id const& other) const;
    bool operator<(level_id const& other) const;

    friend transitions transitions_between(level_id from, level_id const& to, level_counter& c);
    friend struct fmt::formatter<level_id>;
    friend std::ostream& operator<<(std::ostream& os, level_id const& id);

  private:
    std::vector<std::size_t> id_{};
    hash_type hash_;
  };

  level_id id_for(char const* str);
  level_id operator"" _id(char const* str, std::size_t);
  std::ostream& operator<<(std::ostream& os, level_id const& id);
}

namespace fmt {
  template <>
  struct formatter<meld::level_id> : formatter<std::vector<std::size_t>> {
    // parse is inherited from formatter<std::vector<std::size_t>>.
    template <typename FormatContext>
    auto format(meld::level_id const& id, FormatContext& ctx)
    {
      return formatter<std::vector<std::size_t>>::format(id.id_, ctx);
    }
  };
}

namespace std {
  template <>
  struct hash<meld::level_id> {
    std::size_t operator()(meld::level_id const& id) const noexcept { return id.hash(); }
  };
}

#endif /* meld_model_level_id_hpp */
