#ifndef meld_core_concepts_hpp
#define meld_core_concepts_hpp

#include "meld/core/fwd.hpp"
#include "meld/metaprogramming/type_deduction.hpp"

#include <concepts>
#include <utility>

namespace meld {
  // clang-format off
  //  => Apple clang-format 14 does not understand concepts yet
  template <typename T>
  concept not_void = !std::same_as<T, void>;

  template <typename T, std::size_t N>
  concept at_least_n_input_parameters = number_parameters<T> >= N;

  template <typename T>
  concept at_least_one_input_parameter = at_least_n_input_parameters<T, 1>;

  template <typename T>
  concept at_least_two_input_parameters = at_least_n_input_parameters<T, 2>;

  template <typename T>
  concept at_least_one_output_object = number_output_objects<T> >= 1ull;

  template <typename T>
  concept first_input_parameter_is_non_const_lvalue_reference =
    at_least_one_input_parameter<T> &&
    is_non_const_lvalue_reference<parameter_type<0, T>>{};

  template <typename T, typename R>
  concept returns = std::same_as<return_type<T>, R>;

  template <typename T, typename... Args>
  concept expects_input_parameters = at_least_n_input_parameters<T, sizeof...(Args)> &&
                                     check_parameters<T, Args...>{};

  template <typename T>
  concept is_filter_like = at_least_one_input_parameter<T> && returns<T, bool>;

  template <typename T>
  concept is_monitor_like = at_least_one_input_parameter<T> && returns<T, void>;

  template <typename T>
  concept is_output_like = expects_input_parameters<T, product_store const&> && returns<T, void>;

  template <typename T>
  concept is_reduction_like =
    at_least_two_input_parameters<T> &&
    first_input_parameter_is_non_const_lvalue_reference<T> &&
    returns<T, void>; // <= May change if data products can be created per reduction step.

  template <typename T>
  concept is_splitter_like = expects_input_parameters<T, generator&> && returns<T, void>;

  template <typename T>
  concept is_transform_like = at_least_one_input_parameter<T> && at_least_one_output_object<T>;
  // clang-format on
}

#endif /* meld_core_concepts_hpp */
