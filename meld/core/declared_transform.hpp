#ifndef meld_core_declared_transform_hpp
#define meld_core_declared_transform_hpp

#include "meld/core/fwd.h"
#include "meld/core/product_store.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <concepts>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  class declared_transform {
  public:
    declared_transform(std::string name,
                       std::size_t concurrency,
                       std::vector<std::string> input_keys,
                       std::vector<std::string> output_keys);

    virtual ~declared_transform();

    void invoke(product_store& store) const;
    std::string const& name() const noexcept;
    std::size_t concurrency() const noexcept;
    std::vector<std::string> const& input() const noexcept;
    std::vector<std::string> const& output() const noexcept;

  private:
    virtual void invoke_(product_store& store) const = 0;

    std::string name_;
    std::size_t concurrency_;
    std::vector<std::string> input_keys_;
    std::vector<std::string> output_keys_;
  };

  using declared_transform_ptr = std::unique_ptr<declared_transform>;
  using declared_transforms = std::map<std::string, declared_transform_ptr>;

  // Registering concrete transforms

  template <typename T, typename R, typename... Args>
  class incomplete_transform {
    class complete_transform;
    class transform_requires_output;

  public:
    incomplete_transform(user_functions<T>& funcs, std::string name, R (*f)(Args...)) :
      funcs_{funcs}, name_{move(name)}, ft_{f}
    {
    }

    template <typename FT>
    incomplete_transform(user_functions<T>& funcs, std::string name, FT f) :
      funcs_{funcs}, name_{move(name)}, ft_{std::move(f)}
    {
    }

    // Icky?
    incomplete_transform&
    concurrency(std::size_t concurrency)
    {
      concurrency_ = concurrency;
      return *this;
    }

    // FIXME: Should pick a different parameter type
    auto
    input(std::vector<std::string> input_keys)
    {
      if constexpr (std::same_as<R, void>) {
        funcs_.add_transform(name_,
                             std::make_unique<complete_transform>(
                               name_, concurrency_, move(ft_), move(input_keys), {}));
        return;
      }
      else {
        return transform_requires_output{
          funcs_, move(name_), concurrency_, move(ft_), move(input_keys)};
      }
    }

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
  class incomplete_transform<T, R, Args...>::complete_transform : public declared_transform {
  public:
    complete_transform(std::string name,
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

  template <typename T, typename R, typename... Args>
  class incomplete_transform<T, R, Args...>::transform_requires_output {
  public:
    transform_requires_output(user_functions<T>& funcs,
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
      funcs_.add_transform(name_,
                           std::make_unique<complete_transform>(
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
}

#endif /* meld_core_declared_transform_hpp */
