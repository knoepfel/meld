#ifndef meld_utilities_string_literal_hpp
#define meld_utilities_string_literal_hpp

#include <algorithm>
#include <string_view>

namespace meld {
  template <std::size_t N>
  struct string_literal {
    constexpr string_literal(char const (&str)[N]) { std::copy_n(str, N, value); }
    constexpr operator std::string_view() const { return value; }
    char value[N];
  };
}

#endif /* meld_utilities_string_literal_hpp */
