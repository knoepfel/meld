#ifndef meld_metaprogramming_detail_return_type_hpp
#define meld_metaprogramming_detail_return_type_hpp

#include "meld/metaprogramming/detail/basic_concepts.hpp"

namespace meld::detail {
  // ============================================================================
  template <typename T, typename R, typename... Args>
  R return_type_impl(R (T::*)(Args...));

  template <typename T, typename R, typename... Args>
  R return_type_impl(R (T::*)(Args...) const);

  template <typename R, typename... Args>
  R return_type_impl(R (*)(Args...));

  // noexcept overlaods
  template <typename T, typename R, typename... Args>
  R return_type_impl(R (T::*)(Args...) noexcept);

  template <typename T, typename R, typename... Args>
  R return_type_impl(R (T::*)(Args...) const noexcept);

  template <typename R, typename... Args>
  R return_type_impl(R (*)(Args...) noexcept);

  template <has_call_operator T>
  auto return_type_impl(T&&) -> decltype(return_type_impl(&T::operator()));
}

#endif // meld_metaprogramming_detail_return_type_hpp
