#ifndef meld_core_detail_form_input_arguments_hpp
#define meld_core_detail_form_input_arguments_hpp

#include "meld/core/input_arguments.hpp"

#include <cstddef>
#include <string>
#include <tuple>
#include <utility>

namespace meld::detail {
  template <typename T>
  struct will_use {
    using product_type = T;
    std::string name;
  };

  template <typename T>
  struct uses_value {
    T value;
    T const& retrieve(auto const&) const { return value; }
  };

  template <std::size_t I, typename T>
  constexpr std::size_t next_index(T const&)
  {
    return I;
  }

  template <std::size_t I, typename T>
  constexpr std::size_t next_index(std::tuple<expects_message<T, I>> const&)
  {
    return I + 1;
  }

  template <std::size_t I, typename T>
  auto input_argument(T&& t)
  {
    using U = std::decay_t<T>;
    if constexpr (requires { typename U::product_type; }) {
      return std::make_tuple(expects_message<typename U::product_type, I>{t.name});
    }
    else {
      return std::make_tuple(std::move(t));
    }
  }

  template <std::size_t I, typename Head, typename... Tail>
  auto build_input_arguments(Head head, Tail... tail)
  {
    auto next_arg = input_argument<I>(std::move(head));
    if constexpr (sizeof...(Tail) == 0ull) {
      return next_arg;
    }
    else {
      constexpr std::size_t J = next_index<I>(next_arg);
      return std::tuple_cat(next_arg, build_input_arguments<J>(std::move(tail)...));
    }
  }

  template <typename T>
  auto select_input(std::string label)
  {
    return will_use<T>{std::move(label)};
  }

  template <typename T>
  auto select_input(specified_value<T> t)
  {
    return uses_value<T>{std::move(t.value)};
  }

  template <typename InputTypes, typename InputArgs, std::size_t... Is>
  auto form_input_arguments_impl(InputArgs args, std::index_sequence<Is...>)
  {
    return build_input_arguments<0>(
      select_input<std::tuple_element_t<Is, InputTypes>>(std::move(std::get<Is>(args)))...);
  }

  // =====================================================================================

  template <typename InputTypes, typename InputArgs>
  auto form_input_arguments(InputArgs args)
  {
    constexpr auto N = std::tuple_size_v<InputArgs>;
    return detail::form_input_arguments_impl<InputTypes>(move(args), std::make_index_sequence<N>{});
  }
}

#endif /* meld_core_detail_form_input_arguments_hpp */
