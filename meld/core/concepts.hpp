#ifndef meld_core_concepts_hpp
#define meld_core_concepts_hpp

#include "meld/core/fwd.hpp"
#include "meld/utilities/type_deduction.hpp"

#include <concepts>
#include <utility>

namespace meld {
  // FIXME: Check parameters needs to be more robust (e.g. what the lengths of Args and
  //        input_parameters is not the same).
  template <typename T, typename... Args>
  struct check_parameters {
    using input_parameters = decltype(detail::parameter_types(std::declval<T>()));
    template <std::size_t... Is>
    static constexpr bool check_params_for(std::index_sequence<Is...>)
    {
      return std::conjunction_v<std::is_same<std::tuple_element_t<Is, input_parameters>, Args>...>;
    }
    static constexpr bool value = check_params_for(std::index_sequence_for<Args...>{});
  };

  // clang-format off
  //  => Apple clang-format 14 does not understand concepts yet
  template <typename T>
  concept not_void = !std::same_as<T, void>;

  template <typename T>
  concept at_least_one_input_parameter = detail::number_input_parameters<T> >= 1ull;

  template <typename T>
  concept at_least_two_input_parameters = detail::number_input_parameters<T> >= 2ull;

  template <typename T>
  concept at_least_one_output_object = detail::number_output_objects<T> >= 1ull;

  template <typename T, typename R>
  concept returns = requires(T t) {
    {detail::return_type(t)} -> std::same_as<R>;
  };

  template <typename T, typename... Args>
  concept accepts_input_parameters = at_least_one_input_parameter<T> && check_parameters<T, Args...>::value;

  template <typename T>
  concept is_filter_like = at_least_one_input_parameter<T> && returns<T, bool>;

  template <typename T>
  concept is_monitor_like = at_least_one_input_parameter<T> && returns<T, void>;

  template <typename T>
  concept is_output_like = accepts_input_parameters<T, product_store const&> && returns<T, void>;

  template <typename T>
  concept is_transform_like = at_least_one_input_parameter<T> && at_least_one_output_object<T>;

  template <typename T>
  concept is_reduction_like = at_least_one_input_parameter<T>; // && at_least_one_output_object<T>;

  template <typename T>
  concept is_splitter_like = accepts_input_parameters<T, generator&> && returns<T, void>;
  // clang-format on
}

#endif /* meld_core_concepts_hpp */
