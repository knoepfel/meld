#ifndef meld_core_detail_port_names_hpp
#define meld_core_detail_port_names_hpp

#include "meld/core/input_arguments.hpp"
#include "meld/metaprogramming/to_array.hpp"

#include <cstddef>
#include <string>
#include <tuple>
#include <utility>

namespace meld::detail {
  template <typename Head, typename... Tail>
  auto port_names_impl(Head const& head, Tail const&... tail)
  {
    auto result = std::make_tuple(head.name);
    if constexpr (sizeof...(Tail) == 0ull) {
      return result;
    }
    else {
      static_assert(sizeof...(tail) > 0);
      return std::tuple_cat(result, port_names_impl(tail...));
    }
  }

  template <typename InputArgs, std::size_t... Is>
  auto port_names_impl(InputArgs const& inputs, std::index_sequence<Is...>)
  {
    return port_names_impl(std::get<Is>(inputs)...);
  }

  // =====================================================================================

  template <typename InputArgs>
  auto port_names(InputArgs const& args)
  {
    constexpr auto N = std::tuple_size_v<InputArgs>;
    auto names = detail::port_names_impl(args, std::make_index_sequence<N>{});
    return to_array<std::string>(names);
  }
}

#endif /* meld_core_detail_port_names_hpp */
