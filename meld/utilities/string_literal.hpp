#ifndef meld_utilities_string_literal_hpp
#define meld_utilities_string_literal_hpp

#include <algorithm>
#include <array>
#include <string_view>

namespace meld {
  template <std::size_t N>
  struct string_literal {
    constexpr string_literal(char const (&str)[N]) { std::copy_n(str, N, value); }
    constexpr operator std::string_view() const { return value; }
    char value[N];
  };

  namespace detail {
    template <string_literal... Resources>
    consteval bool unique()
    {
      std::array<std::string_view, sizeof...(Resources)> names{std::string_view(Resources)...};
      auto e = end(names);
      std::sort(begin(names), e);
      return std::unique(begin(names), e) == e;
    }
  }

  template <string_literal... Resources>
  concept are_unique = detail::unique<Resources...>();
}

#endif /* meld_utilities_string_literal_hpp */
