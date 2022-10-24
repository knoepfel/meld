#ifndef meld_metaprogramming_type_deduction_hpp
#define meld_metaprogramming_type_deduction_hpp

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

    template <typename T>
    using input_parameter_types = decltype(parameter_types(std::declval<T>()));

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

  template <typename T, typename... Args>
  struct check_parameters {
    using input_parameters = detail::input_parameter_types<T>;
    static_assert(std::tuple_size<input_parameters>{} >= sizeof...(Args));

    template <std::size_t... Is>
    static constexpr bool check_params_for(std::index_sequence<Is...>)
    {
      return std::conjunction_v<std::is_same<std::tuple_element_t<Is, input_parameters>, Args>...>;
    }

    constexpr operator bool() noexcept { return value; }
    static constexpr bool value = check_params_for(std::index_sequence_for<Args...>{});
  };

  // ===================================================================
  template <typename T>
  struct is_non_const_lvalue_reference : std::is_lvalue_reference<T> {
  };

  template <typename T>
  struct is_non_const_lvalue_reference<T const&> : std::false_type {
  };

  // ===================================================================
  template <typename T, std::size_t I>
  using parameter_type_for = std::tuple_element_t<I, detail::input_parameter_types<T>>;
}

#endif /* meld_metaprogramming_type_deduction_hpp */
