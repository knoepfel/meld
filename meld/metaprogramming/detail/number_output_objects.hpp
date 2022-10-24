#ifndef meld_metaprogramming_detail_number_output_objects_hpp
#define meld_metaprogramming_detail_number_output_objects_hpp

#include "meld/metaprogramming/detail/basic_concepts.hpp"

#include <cstddef>
#include <tuple>

namespace meld::detail {
  template <typename T>
  constexpr std::size_t number_types = 1ull;

  template <typename... Args>
  constexpr std::size_t number_types<std::tuple<Args...>> = sizeof...(Args);

  template <typename R>
  constexpr std::size_t number_types_not_void()
  {
    if constexpr (std::is_same<R, void>{}) {
      return 0ull;
    }
    else {
      return number_types<R>;
    }
  }

  // ============================================================================
  // Primary template is assumed to be a lambda
  template <typename T>
  constexpr std::size_t number_output_objects_impl =
    number_output_objects_impl<decltype(&T::operator())>;

  template <typename R, typename T, typename... Args>
  constexpr std::size_t number_output_objects_impl<R (T::*)(Args...)> = number_types_not_void<R>();

  template <typename R, typename T, typename... Args>
  constexpr std::size_t number_output_objects_impl<R (T::*)(Args...) const> =
    number_types_not_void<R>();

  template <typename R, typename... Args>
  constexpr std::size_t number_output_objects_impl<R (*)(Args...)> = number_types_not_void<R>();

  template <typename R, typename... Args>
  constexpr std::size_t number_output_objects_impl<R(Args...)> = number_types_not_void<R>();

  // noexcept specializations
  template <typename R, typename T, typename... Args>
  constexpr std::size_t number_output_objects_impl<R (T::*)(Args...) noexcept> =
    number_types_not_void<R>();

  template <typename R, typename T, typename... Args>
  constexpr std::size_t number_output_objects_impl<R (T::*)(Args...) const noexcept> =
    number_types_not_void<R>();

  template <typename R, typename... Args>
  constexpr std::size_t number_output_objects_impl<R (*)(Args...) noexcept> =
    number_types_not_void<R>();

  template <typename R, typename... Args>
  constexpr std::size_t number_output_objects_impl<R(Args...) noexcept> =
    number_types_not_void<R>();
}

#endif /* meld_metaprogramming_detail_number_output_objects_hpp */
