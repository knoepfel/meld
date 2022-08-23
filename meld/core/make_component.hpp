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

  // ==============================================================================
  // Registering transforms

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
      static_assert(sizeof...(Args) == sizeof...(Ts),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return input(std::vector<std::string>{ts...});
    }

  private:
    user_functions<T>& funcs_;
    std::string name_;
    std::size_t concurrency_{tbb::flow::serial};
    std::function<R(Args...)> ft_;
  };

  template <typename T, typename R, typename... Args>
  class incomplete_function<T, R, Args...>::complete_function : public declared_transform {
  public:
    complete_function(std::string name,
                      std::size_t concurrency,
                      std::function<R(Args...)>&& f,
                      std::vector<std::string> input,
                      std::vector<std::string> output) :
      declared_transform{move(name), concurrency, move(input), move(output)}, ft_{move(f)}
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

  // ==============================================================================
  // Registering reductions

  template <typename T, typename R, typename... Args>
  class incomplete_reduction {
    class complete_reduction;
    class reduction_requires_output;

  public:
    incomplete_reduction(user_functions<T>& funcs,
                         std::string name,
                         void (*f)(R&, Args...),
                         R initializer) :
      funcs_{funcs}, name_{move(name)}, ft_{f}, initializer_{std::move(initializer)}
    {
    }

    template <typename FT>
    incomplete_reduction(user_functions<T>& funcs, std::string name, FT f, R initializer) :
      funcs_{funcs}, name_{move(name)}, ft_{std::move(f)}, initializer_{std::move(initializer)}
    {
    }

    // Icky?
    incomplete_reduction&
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
      static_assert(sizeof...(Args) == sizeof...(Ts),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return input(std::vector<std::string>{ts...});
    }

  private:
    user_functions<T>& funcs_;
    std::string name_;
    std::size_t concurrency_{tbb::flow::serial};
    std::function<void(R&, Args...)> ft_;
    R initializer_;
  };

  template <typename T, typename R, typename... Args>
  class incomplete_reduction<T, R, Args...>::complete_reduction : public declared_reduction {
  public:
    complete_reduction(std::string name,
                       std::size_t concurrency,
                       std::function<void(R&, Args...)>&& f,
                       R initializer,
                       std::vector<std::string> input,
                       std::vector<std::string> output) :
      declared_reduction{move(name), concurrency, move(input), move(output)},
      ft_{move(f)},
      initializer_{std::move(initializer)}
    {
    }

  private:
    void
    call(R& result, product_store const& store) // FIXME: 'icky'
    {
      using types = std::tuple<handle_for<Args>...>;
      auto const handles = [ this, &store ]<std::size_t... Is, typename... Ts>(
        std::index_sequence<Is...>, std::tuple<handle<Ts>...>)
      {
        return std::make_tuple(store.get_handle<Ts>(input()[Is])...);
      }
      (std::index_sequence_for<Args...>{}, types{});
      return std::apply(ft_, std::tuple_cat(std::tie(result), handles));
    }

    void
    invoke_(product_store const& store) override
    {
      auto const& parent = store.parent();
      auto it = results_.find(parent->id());
      if (it == results_.end()) {
        it = results_.insert({parent->id(), std::make_unique<R>(initializer_)}).first;
      }
      call(*it->second, store);
    }

    void
    commit_(product_store& store) override
    {
      auto& result = results_.at(store.id());
      store.add_product(output()[0], *result);
      // Reclaim some memory; would be better to erase the entire entry from the map,
      // but that is not thread-safe.
      result.reset();
    }

    std::function<void(R&, Args...)> ft_;
    R initializer_;
    tbb::concurrent_unordered_map<level_id, std::unique_ptr<R>> results_;
  };

  // ==============================================================================
  // Registering user-functions

  struct void_tag {};

  template <typename T>
  class user_functions {
  public:
    user_functions() = default;
    user_functions() requires(not std::same_as<T, void_tag>) : bound_obj_{std::make_optional<T>()}
    {
    }

    // Transforms
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
        *this, name, [bound_obj = std::move(*bound_obj_), f](Args const&... args) -> R {
          return (bound_obj.*f)(args...);
        }};
    }

    template <typename R, typename... Args>
    auto
    declare_transform(std::string name, R (T::*f)(Args...) const)
    {
      assert(bound_obj_);
      return incomplete_function<T, R, Args...>{
        *this, name, [bound_obj = std::move(*bound_obj_), f](Args const&... args) -> R {
          return (bound_obj.*f)(args...);
        }};
    }

    // Reductions
    template <typename R, typename... Args>
    auto
    declare_reduction(std::string name,
                      void (*f)(R&, Args...),
                      R initializer = {}) requires std::same_as<T, void_tag>
    {
      assert(not bound_obj_);
      return incomplete_reduction{*this, name, f, std::move(initializer)};
    }

    template <typename R, typename... Args>
    auto
    declare_reduction(std::string name, void (T::*f)(R&, Args...), R initializer = {})
    {
      assert(bound_obj_);
      return incomplete_reduction<T, R, Args...>{
        *this,
        name,
        [bound_obj = std::move(*bound_obj_), f](R& result, Args const&... args) {
          return (bound_obj.*f)(result, args...);
        },
        std::move(initializer)};
    }

    template <typename R, typename... Args>
    auto
    declare_reduction(std::string name, void (T::*f)(R&, Args...) const, R initializer = {})
    {
      assert(bound_obj_);
      return incomplete_reduction<T, R, Args...>{
        *this,
        name,
        [bound_obj = std::move(*bound_obj_), f](R& result, Args const&... args) {
          return (bound_obj.*f)(result, args...);
        },
        std::move(initializer)};
    }

    // Expert-use only
    void
    add_function(std::string const& name, declared_transform_ptr ptr)
    {
      funcs_.try_emplace(name, std::move(ptr));
    }

    void
    add_reduction(std::string const& name, declared_reduction_ptr ptr)
    {
      reductions_.try_emplace(name, std::move(ptr));
    }

    declared_transforms&&
    release_transforms()
    {
      return std::move(funcs_);
    }

    declared_reductions&&
    release_reductions()
    {
      return std::move(reductions_);
    }

  private:
    std::optional<T> bound_obj_;
    declared_transforms funcs_;
    declared_reductions reductions_;
  };

  // ================================================================
  // Implementation details of input and function_requires_output

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

  // ================================================================
  // Implementation details of input and reduction_requires_output

  template <typename T, typename R, typename... Args>
  auto
  incomplete_reduction<T, R, Args...>::input(std::vector<std::string> input_keys)
  {
    if constexpr (std::same_as<R, void>) {
      funcs_.add_reduction(
        name_,
        std::make_unique<complete_reduction>(
          name_, concurrency_, move(ft_), std::move(initializer_), move(input_keys), {}));
      return;
    }
    else {
      return reduction_requires_output{
        funcs_, move(name_), concurrency_, move(ft_), std::move(initializer_), move(input_keys)};
    }
  }

  template <typename T, typename R, typename... Args>
  class incomplete_reduction<T, R, Args...>::reduction_requires_output {
  public:
    reduction_requires_output(user_functions<T>& funcs,
                              std::string name,
                              std::size_t concurrency,
                              std::function<void(R&, Args...)>&& f,
                              R initializer,
                              std::vector<std::string> input_keys) :
      funcs_{funcs},
      name_{move(name)},
      concurrency_{concurrency},
      ft_{move(f)},
      initializer_{std::move(initializer)},
      input_keys_{move(input_keys)}
    {
    }

    void
    output(std::vector<std::string> output_keys)
    {
      funcs_.add_reduction(name_,
                           std::make_unique<complete_reduction>(name_,
                                                                concurrency_,
                                                                move(ft_),
                                                                std::move(initializer_),
                                                                move(input_keys_),
                                                                move(output_keys)));
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
    std::function<void(R&, Args...)> ft_;
    R initializer_;
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
