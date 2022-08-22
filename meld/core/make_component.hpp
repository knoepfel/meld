#ifndef meld_core_make_component_hpp
#define meld_core_make_component_hpp

#include "meld/core/framework_graph.hpp"
#include "meld/core/product_store.hpp"
#include "meld/utilities/type_deduction.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <concepts>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  template <typename T>
  class user_functions;

  template <typename T, typename R, typename... Args>
  class incomplete_function {
    class complete_function;
    class function_requires_output;

  public:
    incomplete_function(user_functions<T>& funcs, std::string name, R (*f)(Args...)) :
      funcs_{funcs}, name_{move(name)}, ft_{f}
    {
    }

    template <typename FT>
    incomplete_function(user_functions<T>& funcs, std::string name, FT f) :
      funcs_{funcs}, name_{move(name)}, ft_{std::move(f)}
    {
    }

    // Icky?
    incomplete_function&
    concurrency(std::size_t concurrency)
    {
      concurrency_ = concurrency;
      return *this;
    }

    // FIXME: Should pick a different parameter type
    auto input(std::vector<std::string> input_keys);

    template <typename... Ts>
    auto
    input(Ts... ts)
    {
      static_assert(std::conjunction_v<std::is_convertible<Ts, std::string>...>);
      return input(std::vector<std::string>{ts...});
    }

  private:
    user_functions<T>& funcs_;
    std::string name_;
    std::size_t concurrency_{tbb::flow::serial};
    std::function<R(Args...)> ft_;
  };

  template <typename T, typename R, typename... Args>
  class incomplete_function<T, R, Args...>::complete_function : public declared_function {
  public:
    complete_function(std::string name,
                      std::size_t concurrency,
                      std::function<R(Args...)>&& f,
                      std::vector<std::string> input,
                      std::vector<std::string> output) :
      declared_function{move(name), concurrency, move(input), move(output)}, ft_{move(f)}
    {
    }

    R
    call(product_store& store) const // FIXME: 'icky'
    {
      using types = std::tuple<handle_for<Args>...>;
      auto const handles = [ this, &store ]<std::size_t... Is, typename... Ts>(
        std::index_sequence<Is...>, std::tuple<handle<Ts>...>)
      {
        return std::make_tuple(store.get_handle<Ts>(input()[Is])...);
      }
      (std::index_sequence_for<Args...>{}, types{});
      return std::apply(ft_, handles);
    }

  private:
    void
    invoke_(product_store& store) const override
    {
      if constexpr (std::same_as<R, void>) {
        call(store);
      }
      else {
        auto result = call(store);
        store.add_product(output()[0], std::move(result));
      }
    }

    std::function<R(Args...)> ft_;
  };

  struct void_tag {};

  template <typename T>
  class user_functions {
  public:
    user_functions() = default;
    user_functions() requires(not std::same_as<T, void_tag>) : bound_obj_{std::make_optional<T>()}
    {
    }

    template <typename R, typename... Args>
    auto
    declare_transform(std::string name, R (*f)(Args...)) requires std::same_as<T, void_tag>
    {
      assert(not bound_obj_);
      return incomplete_function{*this, name, f};
    }

    template <typename R, typename... Args>
    auto
    declare_transform(std::string name, R (T::*f)(Args...))
    {
      assert(bound_obj_);
      return incomplete_function<T, R, Args...>{
        *this, name, [bound_obj = std::move(*bound_obj_), f](auto const&... args) {
          return (bound_obj.*f)(args...);
        }};
    }

    template <typename R, typename... Args>
    auto
    declare_transform(std::string name, R (T::*f)(Args...) const)
    {
      assert(bound_obj_);
      return incomplete_function<T, R, Args...>{
        *this, name, [bound_obj = std::move(*bound_obj_), f](auto const&... args) {
          return (bound_obj.*f)(args...);
        }};
    }

    void
    add_function(std::string const& name, declared_function_ptr ptr)
    {
      funcs_.try_emplace(name, std::move(ptr));
    }

    declared_functions&&
    release_functions()
    {
      return std::move(funcs_);
    }

  private:
    std::optional<T> bound_obj_;
    declared_functions funcs_;
  };

  template <typename T, typename R, typename... Args>
  auto
  incomplete_function<T, R, Args...>::input(std::vector<std::string> input_keys)
  {
    if constexpr (std::same_as<R, void>) {
      funcs_.add_function(
        name_,
        std::make_unique<complete_function>(name_, concurrency_, move(ft_), move(input_keys), {}));
      return;
    }
    else {
      return function_requires_output{
        funcs_, move(name_), concurrency_, move(ft_), move(input_keys)};
    }
  }

  template <typename T, typename R, typename... Args>
  class incomplete_function<T, R, Args...>::function_requires_output {
  public:
    function_requires_output(user_functions<T>& funcs,
                             std::string name,
                             std::size_t concurrency,
                             std::function<R(Args...)>&& f,
                             std::vector<std::string> input_keys) :
      funcs_{funcs},
      name_{move(name)},
      concurrency_{concurrency},
      ft_{move(f)},
      input_keys_{move(input_keys)}
    {
    }

    void
    output(std::vector<std::string> output_keys)
    {
      funcs_.add_function(name_,
                          std::make_unique<complete_function>(
                            name_, concurrency_, move(ft_), move(input_keys_), move(output_keys)));
    }

    template <typename... Ts>
    void
    output(Ts... ts)
    {
      static_assert(std::conjunction_v<std::is_convertible<Ts, std::string>...>);
      output(std::vector<std::string>{ts...});
    }

  private:
    user_functions<T>& funcs_;
    std::string name_;
    std::size_t concurrency_;
    std::function<R(Args...)> ft_;
    std::vector<std::string> input_keys_;
  };

  template <typename T = void_tag>
  auto
  make_component()
  {
    if constexpr (std::same_as<T, void_tag>) {
      return user_functions<void_tag>{};
    }
    else {
      return user_functions<T>{};
    }
  }
}

#endif /* meld_core_make_component_hpp */
