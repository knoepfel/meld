#ifndef meld_metaprogramming_index_for_type_hpp
#define meld_metaprogramming_index_for_type_hpp

#include <cstddef>
#include <tuple>

namespace meld {
  namespace detail {
    template <std::size_t I, typename T, typename Tuple>
    constexpr std::size_t index_for_type_impl()
    {
      if constexpr (I >= std::tuple_size_v<Tuple>) {
        return I;
      }
      else if constexpr (std::same_as<T, std::tuple_element_t<I, Tuple>>) {
        return I;
      }
      else {
        return index_for_type_impl<I + 1, T, Tuple>();
      }
    }
  }

  template <typename T, typename Tuple>
  constexpr std::size_t index_for_type()
  {
    return detail::index_for_type_impl<0ull, T, Tuple>();
  }
}

#endif /* meld_metaprogramming_index_for_type_hpp */
