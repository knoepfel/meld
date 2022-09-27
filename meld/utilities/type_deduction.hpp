#ifndef meld_utilities_type_deduction_hpp
#define meld_utilities_type_deduction_hpp

#include <functional>
#include <memory>
#include <tuple>

namespace meld {
  namespace detail {
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

  struct void_tag {};

  template <typename R, typename... Args>
  auto delegate(std::shared_ptr<void_tag>&, R (*f)(Args...))
  {
    return std::function{f};
  }

  template <typename R, typename T, typename... Args>
  auto delegate(std::shared_ptr<T>& obj, R (T::*f)(Args...))
  {
    return std::function{[t = obj, f](Args... args) mutable -> R { return ((*t).*f)(args...); }};
  }

  template <typename R, typename T, typename... Args>
  auto delegate(std::shared_ptr<T>& obj, R (T::*f)(Args...) const)
  {
    return std::function{[t = obj, f](Args... args) mutable -> R { return ((*t).*f)(args...); }};
  }
}

#endif /* meld_utilities_type_deduction_hpp */
