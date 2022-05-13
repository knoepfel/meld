#ifndef meld_utilities_sized_tuple_hpp
#define meld_utilities_sized_tuple_hpp

#include <cstddef>
#include <span>
#include <tuple>
#include <utility>

namespace meld {
  // Infrastructure to allow specification of (e.g.) sized_tuple<T, 4>, which is an alias for
  // std::tuple<T, T, T, T>
  template <typename T, std::size_t>
  using type_t = T;

  template <typename T, std::size_t... I>
  std::tuple<type_t<T, I>...> sized_tuple_for(std::index_sequence<I...>);

  template <typename T, std::size_t N>
  using sized_tuple = decltype(sized_tuple_for<T>(std::make_index_sequence<N>{}));

  template <typename T, std::size_t N>
  constexpr auto
  sized_tuple_from(std::span<T> view)
  {
    return []<std::size_t... I>(auto const& view, std::index_sequence<I...>)
    {
      return std::make_tuple(view[I]...);
    }
    (view, std::make_index_sequence<N>{});
  }

  template <typename... T>
  using concatenated_tuples = decltype(std::tuple_cat(std::declval<T>()...));
}

#endif /* meld_utilities_sized_tuple_hpp */
