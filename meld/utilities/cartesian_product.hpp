#ifndef meld_utilities_cartesian_product_hpp
#define meld_utilities_cartesian_product_hpp

#include <cstddef>
#include <tuple>
#include <utility>

namespace meld {
  namespace detail {
    template <typename T, typename UTup, typename FT, std::size_t... Is>
    void one_by_n(T& t, UTup const& utup, FT const& func, std::index_sequence<Is...>)
    {
      (func(t, std::get<Is>(utup)), ...);
    };
  }

  template <typename... Ts, typename... Us, typename FT>
  void cartesian_product(std::tuple<Ts...> const& ts, std::tuple<Us...> const& us, FT&& func)
  {
    using namespace std;
    []<size_t... Is>(auto const& ttup, auto const& utup, auto const& f, index_sequence<Is...>)
    {
      (detail::one_by_n(get<Is>(ttup), utup, f, index_sequence_for<Us...>{}), ...);
    }
    (ts, us, func, index_sequence_for<Ts...>{});
  }
}

#endif /* meld_utilities_cartesian_product_hpp */
