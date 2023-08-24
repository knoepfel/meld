#ifndef meld_core_input_arguments_hpp
#define meld_core_input_arguments_hpp

#include "meld/core/message.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <tuple>
#include <utility>

namespace meld {
  template <typename T, std::size_t JoinNodePort>
  struct retriever {
    using handle_arg_t = typename handle_for<T>::value_type;
    std::string name;
    auto retrieve(auto const& messages) const
    {
      return std::get<JoinNodePort>(messages).store->template get_handle<handle_arg_t>(name);
    }
  };

  struct specified_label {
    std::string name;
  };

  template <typename InputTypes, std::size_t... Is>
  auto form_input_arguments_impl(std::array<specified_label, sizeof...(Is)> args,
                                 std::index_sequence<Is...>)
  {
    return std::make_tuple(
      retriever<std::tuple_element_t<Is, InputTypes>, Is>{std::move(args[Is].name)}...);
  }

  // =====================================================================================

  template <typename InputTypes, std::size_t N>
  auto form_input_arguments(std::array<specified_label, N> args)
  {
    return form_input_arguments_impl<InputTypes>(std::move(args), std::make_index_sequence<N>{});
  }
}

#endif /* meld_core_input_arguments_hpp */
