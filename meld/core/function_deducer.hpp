#ifndef meld_core_function_deducer_hpp
#define meld_core_function_deducer_hpp

#include "meld/core/product_store.hpp"

#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace meld {

  template <typename R, typename... Args>
  class function_deducer {
  public:
    using types = std::tuple<handle_for<Args>...>;

    template <typename RT, typename... Ts>
    function_deducer(RT (*f)(Ts...), std::vector<std::string> names) : ft_{f}, names_{move(names)}
    {
    }

    R
    call(product_store const& store)
    {
      auto const handles = [ this, &store ]<std::size_t... Is, typename... Ts>(
        std::index_sequence<Is...>, std::tuple<handle<Ts>...>)
      {
        return std::make_tuple(store.get_handle<Ts>(names_[Is])...);
      }
      (std::index_sequence_for<Args...>{}, types{});
      return std::apply(ft_, handles);
    }

  private:
    std::function<R(Args...)> ft_;
    std::vector<std::string> names_;
  };

  template <typename R, typename... Args>
  function_deducer(R (*)(Args...), std::vector<std::string>) -> function_deducer<R, Args...>;
}

#endif /* meld_core_function_deducer_hpp */
