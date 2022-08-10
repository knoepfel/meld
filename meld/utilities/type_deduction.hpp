#ifndef meld_utilities_type_deduction_hpp
#define meld_utilities_type_deduction_hpp

#include <tuple>

namespace meld::detail {
  template <typename T, typename R, typename... Args>
  R return_type_selector(R (T::*)(Args&&...));

  template <typename T, typename R, typename... Args>
  R return_type_selector(R (T::*)(Args&&...) const);

  template <typename T, typename R, typename... Args>
  std::tuple<Args...> parameter_types_selector(R (T::*)(Args&&...));

  template <typename T, typename R, typename... Args>
  std::tuple<Args...> parameter_types_selector(R (T::*)(Args&&...) const);

  template <typename T>
  using return_type = decltype(return_type_selector(&T::operator()));

  template <typename T>
  using parameter_types = decltype(parameter_types_selector(&T::operator()));
}

#endif /* meld_utilities_type_deduction_hpp */
