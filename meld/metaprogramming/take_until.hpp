#ifndef meld_metaprogramming_take_until_hpp
#define meld_metaprogramming_take_until_hpp

#include "meld/metaprogramming/index_for_type.hpp"

#include <tuple>

namespace meld {
  template <typename T, typename... Args>
  auto take_until(std::tuple<Args...> const& input)
  {
    constexpr auto N = index_for_type<T, std::decay_t<decltype(input)>>();
    return []<std::size_t... Is>(auto const& tup, std::index_sequence<Is...>)
    {
      return std::make_tuple(std::get<Is>(tup)...);
    }
    (input, std::make_index_sequence<N>{});
  }
}

#endif /* meld_metaprogramming_take_until_hpp */
