#ifndef meld_utilities_type_deduction_hpp
#define meld_utilities_type_deduction_hpp

#include <functional>
#include <memory>
#include <tuple>

namespace meld {
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

  namespace detail {
    // ============================================================================
    template <typename T, typename R, typename... Args>
    R return_type(R (T::*)(Args...));

    template <typename T, typename R, typename... Args>
    R return_type(R (T::*)(Args...) const);

    template <typename R, typename... Args>
    R return_type(R (*)(Args...));

    // noexcept overlaods
    template <typename T, typename R, typename... Args>
    R return_type(R (T::*)(Args...) noexcept);

    template <typename T, typename R, typename... Args>
    R return_type(R (T::*)(Args...) const noexcept);

    template <typename R, typename... Args>
    R return_type(R (*)(Args...) noexcept);

    // ============================================================================
    template <typename T, typename R, typename... Args>
    std::tuple<Args...> parameter_types(R (T::*)(Args...));

    template <typename T, typename R, typename... Args>
    std::tuple<Args...> parameter_types(R (T::*)(Args...) const);

    template <typename R, typename... Args>
    std::tuple<Args...> parameter_types(R (*)(Args...));

    // noexcept overloads
    template <typename T, typename R, typename... Args>
    std::tuple<Args...> parameter_types(R (T::*)(Args...) noexcept);

    template <typename T, typename R, typename... Args>
    std::tuple<Args...> parameter_types(R (T::*)(Args...) const noexcept);

    template <typename R, typename... Args>
    std::tuple<Args...> parameter_types(R (*)(Args...) noexcept);

    // ============================================================================
    // Primary template is assumed to be a lambda
    template <typename T>
    constexpr std::size_t number_input_parameters =
      number_input_parameters<decltype(&T::operator())>;

    template <typename R, typename T, typename... Args>
    constexpr std::size_t number_input_parameters<R (T::*)(Args...)> = sizeof...(Args);

    template <typename R, typename T, typename... Args>
    constexpr std::size_t number_input_parameters<R (T::*)(Args...) const> = sizeof...(Args);

    template <typename R, typename... Args>
    constexpr std::size_t number_input_parameters<R (*)(Args...)> = sizeof...(Args);

    template <typename R, typename... Args>
    constexpr std::size_t number_input_parameters<R(Args...)> = sizeof...(Args);

    // noexcept specializations
    template <typename R, typename T, typename... Args>
    constexpr std::size_t number_input_parameters<R (T::*)(Args...) noexcept> = sizeof...(Args);

    template <typename R, typename T, typename... Args>
    constexpr std::size_t number_input_parameters<R (T::*)(Args...) const noexcept> =
      sizeof...(Args);

    template <typename R, typename... Args>
    constexpr std::size_t number_input_parameters<R (*)(Args...) noexcept> = sizeof...(Args);

    template <typename R, typename... Args>
    constexpr std::size_t number_input_parameters<R(Args...) noexcept> = sizeof...(Args);

    // ============================================================================
    // Primary template is assumed to be a lambda
    template <typename T>
    constexpr std::size_t number_output_objects = number_output_objects<decltype(&T::operator())>;

    template <typename R, typename T, typename... Args>
    constexpr std::size_t number_output_objects<R (T::*)(Args...)> = number_types_not_void<R>();

    template <typename R, typename T, typename... Args>
    constexpr std::size_t number_output_objects<R (T::*)(Args...) const> =
      number_types_not_void<R>();

    template <typename R, typename... Args>
    constexpr std::size_t number_output_objects<R (*)(Args...)> = number_types_not_void<R>();

    template <typename R, typename... Args>
    constexpr std::size_t number_output_objects<R(Args...)> = number_types_not_void<R>();

    // noexcept specializations
    template <typename R, typename T, typename... Args>
    constexpr std::size_t number_output_objects<R (T::*)(Args...) noexcept> =
      number_types_not_void<R>();

    template <typename R, typename T, typename... Args>
    constexpr std::size_t number_output_objects<R (T::*)(Args...) const noexcept> =
      number_types_not_void<R>();

    template <typename R, typename... Args>
    constexpr std::size_t number_output_objects<R (*)(Args...) noexcept> =
      number_types_not_void<R>();

    template <typename R, typename... Args>
    constexpr std::size_t number_output_objects<R(Args...) noexcept> = number_types_not_void<R>();
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
