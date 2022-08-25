#ifndef meld_core_make_component_hpp
#define meld_core_make_component_hpp

#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/framework_graph.hpp"

#include <concepts>
#include <memory>
#include <string>
#include <tuple>
#include <utility>

namespace meld {

  // ==============================================================================
  // Registering user functions

  struct void_tag {};

  template <typename T>
  class user_functions {
  public:
    user_functions() = default;
    user_functions() requires(not std::same_as<T, void_tag>) : bound_obj_{std::make_unique<T>()} {}

    // Transforms
    template <typename R, typename... Args>
    auto
    declare_transform(std::string name, R (*f)(Args...)) requires std::same_as<T, void_tag>
    {
      assert(not bound_obj_);
      return incomplete_transform{*this, name, f};
    }

    template <typename R, typename... Args>
    auto
    declare_transform(std::string name, R (T::*f)(Args...))
    {
      assert(bound_obj_);
      return incomplete_transform<T, R, Args...>{
        *this, name, [bound_obj = std::move(*bound_obj_), f](Args const&... args) -> R {
          return (bound_obj.*f)(args...);
        }};
    }

    template <typename R, typename... Args>
    auto
    declare_transform(std::string name, R (T::*f)(Args...) const)
    {
      assert(bound_obj_);
      return incomplete_transform<T, R, Args...>{
        *this, name, [bound_obj = std::move(*bound_obj_), f](Args const&... args) -> R {
          return (bound_obj.*f)(args...);
        }};
    }

    // Reductions
    template <typename R, typename... Args, typename... InitArgs>
    auto
    declare_reduction(std::string name,
                      void (*f)(R&, Args...),
                      InitArgs&&... init_args) requires std::same_as<T, void_tag>
    {
      assert(not bound_obj_);
      return incomplete_reduction{
        *this, name, f, std::make_tuple(std::forward<InitArgs>(init_args)...)};
    }

    template <typename R, typename... Args, typename... InitArgs>
    auto
    declare_reduction(std::string name, void (T::*f)(R&, Args...), InitArgs&... init_args)
    {
      assert(bound_obj_);
      return incomplete_reduction<T, R, Args...>{
        *this,
        name,
        [bound_obj = std::move(*bound_obj_), f](R& result, Args const&... args) {
          return (bound_obj.*f)(result, args...);
        },
        std::make_tuple(std::forward<InitArgs>(init_args)...)};
    }

    template <typename R, typename... Args, typename... InitArgs>
    auto
    declare_reduction(std::string name, void (T::*f)(R&, Args...) const, InitArgs&&... init_args)
    {
      assert(bound_obj_);
      return incomplete_reduction<T, R, Args...>{
        *this,
        name,
        [bound_obj = std::move(*bound_obj_), f](R& result, Args const&... args) {
          return (bound_obj.*f)(result, args...);
        },
        std::make_tuple(std::forward<InitArgs>(init_args)...)};
    }

    // Expert-use only
    void
    add_transform(std::string const& name, declared_transform_ptr ptr)
    {
      transforms_.try_emplace(name, std::move(ptr));
    }

    void
    add_reduction(std::string const& name, declared_reduction_ptr ptr)
    {
      reductions_.try_emplace(name, std::move(ptr));
    }

    framework_graph::declared_callbacks
    release_callbacks()
    {
      return {std::move(transforms_), std::move(reductions_)};
    }

  private:
    std::unique_ptr<T> bound_obj_;
    declared_transforms transforms_;
    declared_reductions reductions_;
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
