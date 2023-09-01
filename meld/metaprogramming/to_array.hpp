#ifndef meld_metaprogramming_to_array_hpp
#define meld_metaprogramming_to_array_hpp

#include <tuple>

namespace meld {
  template <typename T, typename... Args>
  auto to_array(std::tuple<Args...> const& tup)
  {
    constexpr auto N = sizeof...(Args);
    return []<std::size_t... Is>(auto const& tup, std::index_sequence<Is...>) {
      return std::array<T, N>{T{std::get<Is>(tup)}...};
    }(tup, std::make_index_sequence<N>{});
  }
}

#endif /* meld_metaprogramming_to_array_hpp */
